/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "DeferredExample.h"
#include "GLTFLoader.h"
using namespace NCL;
using namespace Rendering;

const int LIGHTCOUNT = 64;

struct LightStageUBOData {
	Matrix4 inverseViewProj;
	Vector3 camPosition;
	float	nothing;
	Vector2 screenResolution;
};

DeferredExample::DeferredExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(-20.0f).SetPosition({ 0, 75.0f, 200 }).SetFarPlane(1000.0f);

	cubeMesh	= LoadMesh("Cube.msh");
	sphereMesh	= LoadMesh("Sphere.msh");
	quadMesh	= GenerateQuad();

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix4::Translation({ -50, 10, -50 }) * Matrix4::Scale({ 10.2f, 10.2f, 10.2f });

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform = Matrix4::Scale({ 500.0f, 1.0f, 500.0f });

	objectTextures[0] = VulkanTexture::TextureFromFile("rust_diffuse.png");
	objectTextures[1] = VulkanTexture::TextureFromFile("rust_bump.png");
	objectTextures[2] = VulkanTexture::TextureFromFile("concrete_diffuse.png");
	objectTextures[3] = VulkanTexture::TextureFromFile("concrete_bump.png");

	LoadShaders();
	CreateFrameBuffers(windowWidth, windowHeight);
	BuildGBufferPipeline();
	BuildLightPassPipeline();
	BuildCombinePipeline();
	UpdateDescriptors();

	Light allLights[LIGHTCOUNT];
	for (int i = 0; i < LIGHTCOUNT; ++i) {
		allLights[i].position.x = Maths::RandomValue(-100, 100);
		allLights[i].position.y = Maths::RandomValue(5, 50);
		allLights[i].position.z = Maths::RandomValue(-100, 100);

		allLights[i].colour.x = Maths::RandomValue(0.5f, 1000.0f);
		allLights[i].colour.y = Maths::RandomValue(0.5f, 1000.0f);
		allLights[i].colour.z = Maths::RandomValue(0.5f, 1000.0f);

		allLights[i].radius = Maths::RandomValue(25.0f, 50.0f);
	}

	lightUniform = CreateBuffer(sizeof(allLights), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	UploadBufferData(lightUniform, &allLights, sizeof(allLights));

	lightStageUniform = CreateBuffer(sizeof(LightStageUBOData), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	UpdateBufferDescriptor(*lightStageDescriptor, lightStageUniform, 0, vk::DescriptorType::eUniformBuffer);
	UpdateBufferDescriptor(*lightDescriptor, lightUniform, 0, vk::DescriptorType::eUniformBuffer);
}

void DeferredExample::OnWindowResize(int w, int h) {
	VulkanRenderer::OnWindowResize(w, h);
	CreateFrameBuffers(w,h);
	UpdateDescriptors();
}

void	DeferredExample::LoadShaders() {
	gBufferShader = VulkanShaderBuilder("Deferred GBuffer Fill Shader")
		.WithVertexBinary("DeferredScene.vert.spv")
		.WithFragmentBinary("DeferredScene.frag.spv")
	.Build(device);

	lightingShader = VulkanShaderBuilder("Deferred Lighting Shader")
		.WithVertexBinary("DeferredLight.vert.spv")
		.WithFragmentBinary("DeferredLight.frag.spv")
	.Build(device);

	combineShader = VulkanShaderBuilder("Deferred Combine Shader")
		.WithVertexBinary("DeferredCombine.vert.spv")
		.WithFragmentBinary("DeferredCombine.frag.spv")
	.Build(device);
}

void	DeferredExample::CreateFrameBuffers(uint32_t width, uint32_t height) {
	bufferTextures[0] = VulkanTexture::CreateColourTexture(width, height, "GBuffer Diffuse");
	bufferTextures[1] = VulkanTexture::CreateColourTexture(width, height, "GBuffer Normals");
	bufferTextures[2] = VulkanTexture::CreateDepthTexture(width, height, "GBuffer Depth", false);
	bufferTextures[3] = VulkanTexture::CreateColourTexture(width, height, "Deferred Diffuse");
	bufferTextures[4] = VulkanTexture::CreateColourTexture(width, height, "Deferred Specular");
}

void	DeferredExample::UpdateDescriptors() {
	UpdateImageDescriptor(*lightTextureDescriptor, 0, 0, bufferTextures[1]->GetDefaultView(), *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
	UpdateImageDescriptor(*lightTextureDescriptor, 1, 0, bufferTextures[2]->GetDefaultView(), *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);//???

	UpdateImageDescriptor(*combineTextureDescriptor, 0, 0, bufferTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*combineTextureDescriptor, 1, 0, bufferTextures[3]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*combineTextureDescriptor, 2, 0, bufferTextures[4]->GetDefaultView(), *defaultSampler);
}

void	DeferredExample::BuildGBufferPipeline() {
	objectTextureLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device);

	gBufferPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gBufferShader)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false) //We have two attachments, and we want them both to get added!
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithColourFormats({ bufferTextures[0]->GetFormat(), bufferTextures[1]->GetFormat() })
		.WithDepthFormat(bufferTextures[2]->GetFormat())
		.WithDescriptorSetLayout(*cameraLayout)			//Camera is set 0
		.WithDescriptorSetLayout(*objectTextureLayout)	//Textures are set 1
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build(device, pipelineCache);

	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*objectTextureLayout);
	floorObject.objectDescriptorSet = BuildUniqueDescriptorSet(*objectTextureLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, 0, objectTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, 0, objectTextures[1]->GetDefaultView(), *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, 0, objectTextures[2]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, 0, objectTextures[3]->GetDefaultView(), *defaultSampler);
}

void	DeferredExample::BuildLightPassPipeline() {
	lightLayout = VulkanDescriptorSetLayoutBuilder("All Lights")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
	.Build(device); //Get our camera matrices...

	lightStageLayout = VulkanDescriptorSetLayoutBuilder("Light Stage Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device); //Get our camera matrices...	

	lightTextureLayout = VulkanDescriptorSetLayoutBuilder("GBuffer Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device);

	lightPipeline = VulkanPipelineBuilder("Deferred Lighting Pipeline")
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)
		.WithRaster(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true) //We have two attachments, and we want them both to get added!
		.WithDescriptorSetLayout(*cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(*lightLayout)		//Lights are set 1
		.WithDescriptorSetLayout(*lightStageLayout)	//Light index is set 2
		.WithDescriptorSetLayout(*lightTextureLayout)//Textures are set 3
		.WithColourFormats({ bufferTextures[3]->GetFormat() , bufferTextures[4]->GetFormat() })
	.Build(device, pipelineCache);

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	lightStageDescriptor	= BuildUniqueDescriptorSet(*lightStageLayout);
	lightTextureDescriptor	= BuildUniqueDescriptorSet(*lightTextureLayout);
}

void	DeferredExample::BuildCombinePipeline() {
	combineTextureLayout = VulkanDescriptorSetLayoutBuilder("Deferred Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse GBuffer Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse Lighting Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Specular Lighting Part
	.Build(device);

	combinePipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(combineShader)
		.WithDescriptorSetLayout(*combineTextureLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
	.Build(device, pipelineCache);

	combineTextureDescriptor = BuildUniqueDescriptorSet(*combineTextureLayout);
}

void	DeferredExample::FillGBuffer() {
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.pipeline);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(bufferTextures[0]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(bufferTextures[1]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithDepthAttachment( bufferTextures[2]->GetDefaultView(), vk::ImageLayout::eDepthAttachmentOptimal, true, { 1.0f }, false)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	UpdateBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, defaultCmdBuffer, gBufferPipeline	, 1);
	RenderSingleObject(floorObject	, defaultCmdBuffer, gBufferPipeline	, 1);

	EndRendering(defaultCmdBuffer);
}

void	DeferredExample::RenderLights() {	//Second step: Take in the GBUffer and render lights
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *lightPipeline.pipeline);
	TransitionColourToSampler(&*bufferTextures[1], defaultCmdBuffer);
	TransitionDepthToSampler( &*bufferTextures[2], defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(bufferTextures[3]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(bufferTextures[4]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithRenderArea(defaultScreenRect)
		.WithLayerCount(1)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 2, 1, &*lightStageDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 3, 1, &*lightTextureDescriptor, 0, nullptr);

	SubmitDrawCall(*sphereMesh, defaultCmdBuffer, LIGHTCOUNT);

	EndRendering(defaultCmdBuffer);
}

void	DeferredExample::CombineBuffers() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	TransitionColourToSampler(&*bufferTextures[0], defaultCmdBuffer);
	TransitionColourToSampler(&*bufferTextures[3], defaultCmdBuffer);
	TransitionColourToSampler(&*bufferTextures[4], defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *combinePipeline.pipeline);

	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*combineTextureDescriptor, 0, nullptr);

	SubmitDrawCall(*quadMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);

	TransitionSamplerToColour(&*bufferTextures[0], defaultCmdBuffer);
	TransitionSamplerToColour(&*bufferTextures[1], defaultCmdBuffer);

	TransitionSamplerToDepth( &*bufferTextures[2], defaultCmdBuffer);

	TransitionSamplerToColour(&*bufferTextures[3], defaultCmdBuffer);
	TransitionSamplerToColour(&*bufferTextures[4], defaultCmdBuffer);
}

void DeferredExample::RenderFrame() {
	UploadCameraUniform();
	FillGBuffer();
	RenderLights();
	CombineBuffers();
}

void DeferredExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	LightStageUBOData newData;
	newData.camPosition = cameraUniform.camera.GetPosition();
	newData.screenResolution.x = 1.0f / (float)windowWidth;
	newData.screenResolution.y = 1.0f / (float)windowHeight;
	newData.inverseViewProj = (cameraUniform.camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()) *
		cameraUniform.camera.BuildViewMatrix()).Inverse();
	UploadBufferData(lightStageUniform, &newData, sizeof(LightStageUBOData));
}
//Old tutorial was 264 LOC...