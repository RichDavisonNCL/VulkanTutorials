/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "DeferredExample.h"
#include "../GLTFLoader/GLTFLoader.h"
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
	autoBeginDynamicRendering = false;
}

void DeferredExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	camera.SetPitch(-20.0f).SetPosition({ 0, 75.0f, 200 }).SetFarPlane(1000.0f);

	cubeMesh	= LoadMesh("Cube.msh");
	sphereMesh	= LoadMesh("Sphere.msh");
	quadMesh	= GenerateQuad();

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix4::Translation({ -50, 10, -50 }) * Matrix4::Scale({ 10.2f, 10.2f, 10.2f });

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform = Matrix4::Scale({ 500.0f, 1.0f, 500.0f });

	objectTextures[0] = VulkanTexture::TextureFromFile(this, "rust_diffuse.png");
	objectTextures[1] = VulkanTexture::TextureFromFile(this, "rust_bump.png");
	objectTextures[2] = VulkanTexture::TextureFromFile(this, "concrete_diffuse.png");
	objectTextures[3] = VulkanTexture::TextureFromFile(this, "concrete_bump.png");

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

	lightUniform = VulkanBufferBuilder(sizeof(allLights), "Lights")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	lightStageUniform = VulkanBufferBuilder(sizeof(LightStageUBOData), "Light Stage")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	lightUniform.CopyData(&allLights, sizeof(allLights));

	UpdateBufferDescriptor(*lightStageDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightStageUniform);
	UpdateBufferDescriptor(*lightDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightUniform);
}

void DeferredExample::OnWindowResize(int w, int h) {
	VulkanRenderer::OnWindowResize(w, h);
	CreateFrameBuffers(w,h);
}

void	DeferredExample::LoadShaders() {
	gBufferShader = VulkanShaderBuilder("Deferred GBuffer Fill Shader")
		.WithVertexBinary("DeferredScene.vert.spv")
		.WithFragmentBinary("DeferredScene.frag.spv")
	.Build(GetDevice());

	lightingShader = VulkanShaderBuilder("Deferred Lighting Shader")
		.WithVertexBinary("DeferredLight.vert.spv")
		.WithFragmentBinary("DeferredLight.frag.spv")
	.Build(GetDevice());

	combineShader = VulkanShaderBuilder("Deferred Combine Shader")
		.WithVertexBinary("DeferredCombine.vert.spv")
		.WithFragmentBinary("DeferredCombine.frag.spv")
	.Build(GetDevice());
}

void	DeferredExample::CreateFrameBuffers(uint32_t width, uint32_t height) {
	bufferTextures[0] = VulkanTexture::CreateColourTexture(this, width, height, "GBuffer Diffuse");
	bufferTextures[1] = VulkanTexture::CreateColourTexture(this, width, height, "GBuffer Normals");
	bufferTextures[2] = VulkanTexture::CreateDepthTexture(this, width, height, "GBuffer Depth", false);
	bufferTextures[3] = VulkanTexture::CreateColourTexture(this, width, height, "Deferred Diffuse");
	bufferTextures[4] = VulkanTexture::CreateColourTexture(this, width, height, "Deferred Specular");
}

void	DeferredExample::UpdateDescriptors() {
	UpdateImageDescriptor(*lightTextureDescriptor, 0, 0, *bufferTextures[1], *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
	UpdateImageDescriptor(*lightTextureDescriptor, 1, 0, *bufferTextures[2], *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);//???

	UpdateImageDescriptor(*combineTextureDescriptor, 0, 0, *bufferTextures[0], *defaultSampler);
	UpdateImageDescriptor(*combineTextureDescriptor, 1, 0, *bufferTextures[3], *defaultSampler);
	UpdateImageDescriptor(*combineTextureDescriptor, 2, 0, *bufferTextures[4], *defaultSampler);
}

void	DeferredExample::BuildGBufferPipeline() {
	objectTextureLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	gBufferPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gBufferShader)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false) //We have two attachments, and we want them both to get added!
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithColourFormats({ bufferTextures[0]->GetFormat(), bufferTextures[1]->GetFormat() })
		.WithDepthFormat(bufferTextures[2]->GetFormat())
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*objectTextureLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build(GetDevice());

	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*objectTextureLayout);
	floorObject.objectDescriptorSet = BuildUniqueDescriptorSet(*objectTextureLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, 0, *objectTextures[0], *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, 0, *objectTextures[1], *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, 0, *objectTextures[2], *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, 0, *objectTextures[3], *defaultSampler);
}

void	DeferredExample::BuildLightPassPipeline() {
	lightLayout = VulkanDescriptorSetLayoutBuilder("All Lights")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice()); //Get our camera matrices...

	lightStageLayout = VulkanDescriptorSetLayoutBuilder("Light Stage Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice()); //Get our camera matrices...	

	lightTextureLayout = VulkanDescriptorSetLayoutBuilder("GBuffer Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	lightPipeline = VulkanPipelineBuilder("Deferred Lighting Pipeline")
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)
		.WithRaster(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)
		.WithColourFormats({ bufferTextures[3]->GetFormat() , bufferTextures[4]->GetFormat() })
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true) //We have two attachments, and we want them both to get added!
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*lightLayout)
		.WithDescriptorSetLayout(2,*lightStageLayout)
		.WithDescriptorSetLayout(3,*lightTextureLayout)
	.Build(GetDevice());

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	lightStageDescriptor	= BuildUniqueDescriptorSet(*lightStageLayout);
	lightTextureDescriptor	= BuildUniqueDescriptorSet(*lightTextureLayout);
}

void	DeferredExample::BuildCombinePipeline() {
	combineTextureLayout = VulkanDescriptorSetLayoutBuilder("Deferred Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse GBuffer Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse Lighting Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Specular Lighting Part
	.Build(GetDevice());

	combinePipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(combineShader)
		.WithDescriptorSetLayout(0,*combineTextureLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
	.Build(GetDevice());

	combineTextureDescriptor = BuildUniqueDescriptorSet(*combineTextureLayout);
}

void DeferredExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	LightStageUBOData newData;
	newData.camPosition = camera.GetPosition();
	newData.screenResolution.x = 1.0f / (float)(windowWidth );
	newData.screenResolution.y = 1.0f / (float)(windowHeight );
	newData.inverseViewProj = (camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()) *
		camera.BuildViewMatrix()).Inverse();

	lightStageUniform.CopyData(&newData, sizeof(LightStageUBOData));
}

void DeferredExample::RenderFrame() {
	UploadCameraUniform();
	FillGBuffer();
	RenderLights();
	CombineBuffers();
}

void	DeferredExample::FillGBuffer() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferPipeline);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(*bufferTextures[0], vk::ImageLayout::eColorAttachmentOptimal, true, Vulkan::ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(*bufferTextures[1], vk::ImageLayout::eColorAttachmentOptimal, true, Vulkan::ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithDepthAttachment( *bufferTextures[2], vk::ImageLayout::eDepthAttachmentOptimal, true, { 1.0f }, false)
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(frameCmds);

	UpdateBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, frameCmds, gBufferPipeline	, 1);
	RenderSingleObject(floorObject	, frameCmds, gBufferPipeline	, 1);

	EndRendering(frameCmds);
}

void	DeferredExample::RenderLights() {	//Second step: Take in the GBUffer and render lights
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, lightPipeline);
	Vulkan::TransitionColourToSampler(frameCmds, *bufferTextures[1]);
	Vulkan::TransitionDepthToSampler( frameCmds, *bufferTextures[2]);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(*bufferTextures[3], vk::ImageLayout::eColorAttachmentOptimal, true, Vulkan::ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(*bufferTextures[4], vk::ImageLayout::eColorAttachmentOptimal, true, Vulkan::ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithRenderArea(defaultScreenRect)
		.WithLayerCount(1)
	.BeginRendering(frameCmds);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 2, 1, &*lightStageDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 3, 1, &*lightTextureDescriptor, 0, nullptr);

	SubmitDrawCall(frameCmds, *sphereMesh, LIGHTCOUNT);

	EndRendering(frameCmds);
}

void	DeferredExample::CombineBuffers() {
	Vulkan::TransitionColourToSampler(frameCmds, *bufferTextures[0]);
	Vulkan::TransitionColourToSampler(frameCmds, *bufferTextures[3]);
	Vulkan::TransitionColourToSampler(frameCmds, *bufferTextures[4]);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, combinePipeline);

	BeginDefaultRendering(frameCmds);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*combineTextureDescriptor, 0, nullptr);

	SubmitDrawCall(frameCmds, *quadMesh);

	EndRendering(frameCmds);

	Vulkan::TransitionSamplerToColour(frameCmds, *bufferTextures[0]);
	Vulkan::TransitionSamplerToColour(frameCmds, *bufferTextures[1]);

	Vulkan::TransitionSamplerToDepth( frameCmds, *bufferTextures[2]);

	Vulkan::TransitionSamplerToColour(frameCmds, *bufferTextures[3]);
	Vulkan::TransitionSamplerToColour(frameCmds, *bufferTextures[4]);
}
//Old tutorial was 264 LOC...