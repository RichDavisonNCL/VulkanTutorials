/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
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
	cameraUniform.camera.SetPitch(-20.0f).SetPosition(Vector3(0, 75.0f, 200)).SetFarPlane(1000.0f);

	cubeMesh	= MakeSmartMesh(LoadMesh("Cube.msh"));
	sphereMesh	= MakeSmartMesh(LoadMesh("Sphere.msh"));
	quadMesh	= MakeSmartMesh(GenerateQuad());

	boxObject.mesh		= cubeMesh.get();
	boxObject.transform = Matrix4::Translation(Vector3(-50, 10, -50)) * Matrix4::Scale(Vector3(10.2f, 10.2f, 10.2f));

	floorObject.mesh		= cubeMesh.get();
	floorObject.transform	= Matrix4::Scale(Vector3(100.0f, 1.0f, 100.0f));

	objectTextures[0] = VulkanTexture::TexturePtrFromFilename("rust_diffuse.png");
	objectTextures[1] = VulkanTexture::TexturePtrFromFilename("rust_bump.png");
	objectTextures[2] = VulkanTexture::TexturePtrFromFilename("concrete_diffuse.png");
	objectTextures[3] = VulkanTexture::TexturePtrFromFilename("concrete_bump.png");

	LoadShaders();
	CreateFrameBuffers();
	BuildGBufferPipeline();
	BuildLightPassPipeline();
	BuildCombinePipeline();

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

	UpdateUniformBufferDescriptor(*lightStageDescriptor, lightStageUniform, 0);
	UpdateUniformBufferDescriptor(*lightDescriptor, lightUniform, 0);
	UpdateUniformBufferDescriptor(*lightStageCamDescriptor, cameraUniform.cameraData, 0);
}

DeferredExample::~DeferredExample() {
}

void	DeferredExample::LoadShaders() {
	gBufferShader = VulkanShaderBuilder("Deferred GBuffer Fill Shader")
		.WithVertexBinary("DeferredScene.vert.spv")
		.WithFragmentBinary("DeferredScene.frag.spv")
	.BuildUnique(device);

	lightingShader = VulkanShaderBuilder("Deferred Lighting Shader")
		.WithVertexBinary("DeferredLight.vert.spv")
		.WithFragmentBinary("DeferredLight.frag.spv")
	.BuildUnique(device);

	combineShader = VulkanShaderBuilder("Deferred Combine Shader")
		.WithVertexBinary("DeferredCombine.vert.spv")
		.WithFragmentBinary("DeferredCombine.frag.spv")
	.BuildUnique(device);
}

void	DeferredExample::CreateFrameBuffers() {
	bufferTextures[0] = VulkanTexture::GenerateColourTexturePtr(windowWidth, windowHeight, "GBufferDiffuse");
	bufferTextures[1] = VulkanTexture::GenerateColourTexturePtr(windowWidth, windowHeight, "GBufferNormals");
	bufferTextures[2] = VulkanTexture::GenerateDepthTexturePtr(windowWidth, windowHeight, "GBufferDepth", false);
	bufferTextures[3] = VulkanTexture::GenerateColourTexturePtr(windowWidth, windowHeight, "DeferredDiffuse");
	bufferTextures[4] = VulkanTexture::GenerateColourTexturePtr(windowWidth, windowHeight, "DeferredSpecular");
}

void	DeferredExample::BuildGBufferPipeline() {
	cameraLayout = VulkanDescriptorSetLayoutBuilder("CameraMatrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.BuildUnique(device); //Get our camera matrices...

	objectTextureLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device);

	gBufferPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexSpecification(cubeMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(gBufferShader)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false)
		.WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false) //We have two attachments, and we want them both to get added!
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithColourFormats({ bufferTextures[0]->GetFormat(), bufferTextures[1]->GetFormat() })
		.WithDepthStencilFormat(bufferTextures[2]->GetFormat())
		.WithDescriptorSetLayout(cameraLayout)			//Camera is set 0
		.WithDescriptorSetLayout(objectTextureLayout)	//Textures are set 1
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build(device, pipelineCache);

	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*objectTextureLayout);
	floorObject.objectDescriptorSet = BuildUniqueDescriptorSet(*objectTextureLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, objectTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, objectTextures[1]->GetDefaultView(), *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, objectTextures[2]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, objectTextures[3]->GetDefaultView(), *defaultSampler);

	cameraDescriptor = BuildUniqueDescriptorSet(*cameraLayout);
}

void	DeferredExample::BuildLightPassPipeline() {
	lightStageCamLayout = VulkanDescriptorSetLayoutBuilder("CameraMatrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.BuildUnique(device); //Get our camera matrices...

	lightLayout = VulkanDescriptorSetLayoutBuilder("All Lights")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device); //Get our camera matrices...

	lightStageLayout = VulkanDescriptorSetLayoutBuilder("Light Stage Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device); //Get our camera matrices...	

	lightTextureLayout = VulkanDescriptorSetLayoutBuilder("GBuffer Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device);

	lightPipeline = VulkanPipelineBuilder("Deferred Lighting Pipeline")
		.WithVertexSpecification(sphereMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(lightingShader)
		.WithRaster(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true) //We have two attachments, and we want them both to get added!
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(int))
		.WithDescriptorSetLayout(cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(lightLayout)		//Lights are set 1
		.WithDescriptorSetLayout(lightStageLayout)	//Light index is set 2
		.WithDescriptorSetLayout(lightTextureLayout)//Textures are set 3
		.WithColourFormats({ bufferTextures[3]->GetFormat() , bufferTextures[4]->GetFormat() })
	.Build(device, pipelineCache);

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	lightStageDescriptor	= BuildUniqueDescriptorSet(*lightStageLayout);
	lightTextureDescriptor	= BuildUniqueDescriptorSet(*lightTextureLayout);
	lightStageCamDescriptor = BuildUniqueDescriptorSet(*lightStageCamLayout);

	UpdateImageDescriptor(*lightTextureDescriptor, 0, bufferTextures[1]->GetDefaultView(), *defaultSampler,  vk::ImageLayout::eShaderReadOnlyOptimal);
	UpdateImageDescriptor(*lightTextureDescriptor, 1, bufferTextures[2]->GetDefaultView(), *defaultSampler,  vk::ImageLayout::eDepthStencilReadOnlyOptimal);//???
}

void	DeferredExample::BuildCombinePipeline() {
	combineTextureLayout = VulkanDescriptorSetLayoutBuilder("Deferred Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse GBuffer Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse Lighting Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Specular Lighting Part
	.BuildUnique(device);

	combinePipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexSpecification(quadMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShaderState(combineShader)
		.WithDescriptorSetLayout(combineTextureLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
	.Build(device, pipelineCache);

	combineTextureDescriptor = BuildUniqueDescriptorSet(*combineTextureLayout);

	UpdateImageDescriptor(*combineTextureDescriptor, 0, bufferTextures[0]->GetDefaultView(), *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
	UpdateImageDescriptor(*combineTextureDescriptor, 1, bufferTextures[3]->GetDefaultView(), *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
	UpdateImageDescriptor(*combineTextureDescriptor, 2, bufferTextures[4]->GetDefaultView(), *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void	DeferredExample::FillGBuffer() {
	VulkanDynamicRenderBuilder()
		.WithColourAttachment(bufferTextures[0]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(bufferTextures[1]->GetDefaultView(), vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.0f, 0.0f, 0.0f, 1.0f))
		.WithDepthAttachment( bufferTextures[2]->GetDefaultView(), vk::ImageLayout::eDepthAttachmentOptimal, true, { 1.0f }, false)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferPipeline.pipeline.get());
	
	UpdateUniformBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gBufferPipeline.layout.get(), 0, 1, &*cameraDescriptor, 0, nullptr);

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

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 0, 1, &*lightStageCamDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 2, 1, &*lightStageDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 3, 1, &*lightTextureDescriptor, 0, nullptr);

	for (int i = 0; i < LIGHTCOUNT; ++i) {
		defaultCmdBuffer.pushConstants(*lightPipeline.layout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
			, 0, sizeof(int), (void*)&i);
		SubmitDrawCall(sphereMesh.get(), defaultCmdBuffer);
	}
	EndRendering(defaultCmdBuffer);
}

void	DeferredExample::CombineBuffers() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	TransitionColourToSampler(&*bufferTextures[0], defaultCmdBuffer);
	TransitionColourToSampler(&*bufferTextures[3], defaultCmdBuffer);
	TransitionColourToSampler(&*bufferTextures[4], defaultCmdBuffer);

	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *combinePipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*combineTextureDescriptor, 0, nullptr);

	SubmitDrawCall(&*quadMesh, defaultCmdBuffer);

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