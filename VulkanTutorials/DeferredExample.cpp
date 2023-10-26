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
using namespace Vulkan;

const int LIGHTCOUNT = 64;

struct LightStageUBOData {
	Matrix4 inverseViewProj;
	Vector3 camPosition;
	float	nothing;
	Vector2 screenResolution;
};

const int GBUFFER_ALBEDO	= 0;
const int GBUFFER_NORMALS	= 1;
const int GBUFFER_DEPTH		= 2;
const int LIGHTING_DIFFUSE	= 3;
const int LIGHTING_SPECULAR = 4;

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

	objectTextures[0] = LoadTexture("rust_diffuse.png");
	objectTextures[1] = LoadTexture("rust_bump.png");
	objectTextures[2] = LoadTexture("concrete_diffuse.png");
	objectTextures[3] = LoadTexture("concrete_bump.png");

	LoadShaders();
	CreateFrameBuffers(windowSize.x, windowSize.y);
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

	lightUniform = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(allLights), "Lights");

	lightStageUniform = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(LightStageUBOData), "Light Stage");

	lightUniform.CopyData(&allLights, sizeof(allLights));

	WriteBufferDescriptor(*lightStageDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightStageUniform);
	WriteBufferDescriptor(*lightDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightUniform);

	UpdateDescriptors();
}

void DeferredExample::OnWindowResize(int w, int h) {
	VulkanRenderer::OnWindowResize(w, h);
	CreateFrameBuffers(w,h);
}

void	DeferredExample::LoadShaders() {
	gBufferShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("DeferredScene.vert.spv")
		.WithFragmentBinary("DeferredScene.frag.spv")
	.Build("Deferred GBuffer Fill Shader");

	lightingShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("DeferredLight.vert.spv")
		.WithFragmentBinary("DeferredLight.frag.spv")
	.Build("Deferred Lighting Shader");

	combineShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("DeferredCombine.vert.spv")
		.WithFragmentBinary("DeferredCombine.frag.spv")
	.Build("Deferred Combine Shader");
}

void	DeferredExample::CreateFrameBuffers(uint32_t width, uint32_t height) {
	TextureBuilder builder(GetDevice(), GetMemoryAllocator());

	builder
		.WithPool(GetCommandPool(CommandBuffer::Graphics))
		.WithQueue(GetQueue(CommandBuffer::Graphics))
		.WithDimension(width, height, 1)
		.WithMips(false)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithPipeFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.WithFormat(vk::Format::eB8G8R8A8Unorm);

	bufferTextures[GBUFFER_ALBEDO]		= builder.Build("GBuffer Diffuse");
	bufferTextures[GBUFFER_NORMALS]		= builder.Build("GBuffer Normals");
	bufferTextures[LIGHTING_DIFFUSE]	= builder.Build("Deferred Diffuse");
	bufferTextures[LIGHTING_SPECULAR]	= builder.Build("Deferred Specular");

	bufferTextures[GBUFFER_DEPTH] = builder
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
	.Build("GBuffer Depth");

	UpdateDescriptors();
}

void	DeferredExample::UpdateDescriptors() {
	if (!lightTextureDescriptor) {
		return;
	}
	WriteImageDescriptor(*lightTextureDescriptor, 0, 0, *bufferTextures[1], *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
	WriteImageDescriptor(*lightTextureDescriptor, 1, 0, *bufferTextures[2], *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);//???

	WriteImageDescriptor(*combineTextureDescriptor, 0, 0, *bufferTextures[0], *defaultSampler);
	WriteImageDescriptor(*combineTextureDescriptor, 1, 0, *bufferTextures[3], *defaultSampler);
	WriteImageDescriptor(*combineTextureDescriptor, 2, 0, *bufferTextures[4], *defaultSampler);
}

void	DeferredExample::BuildGBufferPipeline() {
	objectTextureLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Object Textures");

	gBufferPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gBufferShader)
		.WithColourAttachment(bufferTextures[GBUFFER_ALBEDO]->GetFormat())
		.WithColourAttachment(bufferTextures[GBUFFER_NORMALS]->GetFormat())
		.WithDepthAttachment(bufferTextures[GBUFFER_DEPTH]->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*objectTextureLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build("Main Scene Pipeline");

	boxObject.descriptorSet	= BuildUniqueDescriptorSet(*objectTextureLayout);
	floorObject.descriptorSet = BuildUniqueDescriptorSet(*objectTextureLayout);

	WriteImageDescriptor(*boxObject.descriptorSet, 0, 0, *objectTextures[0], *defaultSampler);
	WriteImageDescriptor(*boxObject.descriptorSet, 1, 0, *objectTextures[1], *defaultSampler);

	WriteImageDescriptor(*floorObject.descriptorSet, 0, 0, *objectTextures[2], *defaultSampler);
	WriteImageDescriptor(*floorObject.descriptorSet, 1, 0, *objectTextures[3], *defaultSampler);
}

void	DeferredExample::BuildLightPassPipeline() {
	lightLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
	.Build("All Lights"); //Get our camera matrices...

	lightStageLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Light Stage Data"); //Get our camera matrices...	

	lightTextureLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("GBuffer Textures");

	lightPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)
		.WithRaster(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)

		.WithColourAttachment(bufferTextures[LIGHTING_DIFFUSE]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)
		.WithColourAttachment(bufferTextures[LIGHTING_SPECULAR]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)

		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*lightLayout)
		.WithDescriptorSetLayout(2,*lightStageLayout)
		.WithDescriptorSetLayout(3,*lightTextureLayout)
	.Build("Deferred Lighting Pipeline");

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	lightStageDescriptor	= BuildUniqueDescriptorSet(*lightStageLayout);
	lightTextureDescriptor	= BuildUniqueDescriptorSet(*lightTextureLayout);
}

void	DeferredExample::BuildCombinePipeline() {
	combineTextureLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse GBuffer Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Diffuse Lighting Part
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	//Specular Lighting Part
	.Build("Deferred Textures");

	combinePipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(combineShader)		
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0,*combineTextureLayout)

	.Build("Post process pipeline");

	combineTextureDescriptor = BuildUniqueDescriptorSet(*combineTextureLayout);
}

void DeferredExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	LightStageUBOData newData;
	newData.camPosition = camera.GetPosition();
	newData.screenResolution.x = 1.0f / (float)(windowSize.x);
	newData.screenResolution.y = 1.0f / (float)(windowSize.y);
	newData.inverseViewProj = (camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()) *
		camera.BuildViewMatrix()).Inverse();

	lightStageUniform.CopyData(&newData, sizeof(LightStageUBOData));
}

void DeferredExample::RenderFrame() {
	FillGBuffer();
	RenderLights();
	CombineBuffers();
}

void	DeferredExample::FillGBuffer() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferPipeline);

	DynamicRenderBuilder()
		.WithColourAttachment(*bufferTextures[GBUFFER_ALBEDO], vk::ImageLayout::eColorAttachmentOptimal, true, vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(*bufferTextures[GBUFFER_NORMALS], vk::ImageLayout::eColorAttachmentOptimal, true, vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f))
		.WithDepthAttachment( *bufferTextures[GBUFFER_DEPTH], vk::ImageLayout::eDepthAttachmentOptimal, true, { 1.0f }, false)
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(frameCmds);

	WriteBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, frameCmds, gBufferPipeline	, 1);
	RenderSingleObject(floorObject	, frameCmds, gBufferPipeline	, 1);

	frameCmds.endRendering();
}

void	DeferredExample::RenderLights() {	//Second step: Take in the GBUffer and render lights
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, lightPipeline);
	TransitionColourToSampler(frameCmds, *bufferTextures[GBUFFER_NORMALS]);
	TransitionDepthToSampler( frameCmds, *bufferTextures[GBUFFER_DEPTH]);

	DynamicRenderBuilder()
		.WithColourAttachment(*bufferTextures[LIGHTING_DIFFUSE], vk::ImageLayout::eColorAttachmentOptimal, true, vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f))
		.WithColourAttachment(*bufferTextures[LIGHTING_SPECULAR], vk::ImageLayout::eColorAttachmentOptimal, true, vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f))
		.WithRenderArea(defaultScreenRect)
		.WithLayerCount(1)
	.BeginRendering(frameCmds);

	vk::DescriptorSet sets[] = {
		*cameraDescriptor,		//Set 0
		*lightDescriptor,		//Set 1
		*lightStageDescriptor,	//Set 2
		*lightTextureDescriptor //Set 3
	};

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 0, 4, sets, 0, nullptr);

	DrawMesh(frameCmds, *sphereMesh, LIGHTCOUNT);

	frameCmds.endRendering();
}

void	DeferredExample::CombineBuffers() {
	TransitionColourToSampler(frameCmds, *bufferTextures[GBUFFER_ALBEDO]);
	TransitionColourToSampler(frameCmds, *bufferTextures[LIGHTING_DIFFUSE]);
	TransitionColourToSampler(frameCmds, *bufferTextures[LIGHTING_SPECULAR]);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, combinePipeline);

	BeginDefaultRendering(frameCmds);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*combineTextureDescriptor, 0, nullptr);

	DrawMesh(frameCmds, *quadMesh);

	frameCmds.endRendering();

	TransitionSamplerToColour(frameCmds, *bufferTextures[GBUFFER_ALBEDO]);
	TransitionSamplerToColour(frameCmds, *bufferTextures[GBUFFER_NORMALS]);

	TransitionSamplerToDepth( frameCmds, *bufferTextures[GBUFFER_DEPTH]);

	TransitionSamplerToColour(frameCmds, *bufferTextures[LIGHTING_DIFFUSE]);
	TransitionSamplerToColour(frameCmds, *bufferTextures[LIGHTING_SPECULAR]);
}
//Old tutorial was 264 LOC...