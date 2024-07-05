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

TUTORIAL_ENTRY(DeferredExample)

const int LIGHTCOUNT = 64;

struct LightStageUBOData {
	Matrix4 inverseViewProj;
	Vector3 camPosition;
	float	nothing;
	Vector2 screenResolution;
};

DeferredExample::DeferredExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {	
	vkInit.autoBeginDynamicRendering = false;
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	FrameState const& frameState = renderer->GetFrameState();

	camera.SetPitch(-20.0f).SetPosition({ 0, 75.0f, 200 }).SetFarPlane(1000.0f);

	cubeMesh	= LoadMesh("Cube.msh");
	sphereMesh	= LoadMesh("Sphere.msh");
	quadMesh	= GenerateQuad();

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix::Translation(Vector3{ -50, 10, -50 }) * Matrix::Scale(Vector3{ 10.2f, 10.2f, 10.2f });

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform = Matrix::Scale(Vector3{ 500.0f, 1.0f, 500.0f });

	objectTextures[0] = LoadTexture("rust_diffuse.png");
	objectTextures[1] = LoadTexture("rust_bump.png");
	objectTextures[2] = LoadTexture("concrete_diffuse.png");
	objectTextures[3] = LoadTexture("concrete_bump.png");

	LoadShaders();
	CreateFrameBuffers(hostWindow.GetScreenSize().x, hostWindow.GetScreenSize().y);
	BuildPipelines();

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

	lightUniform = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(allLights), "Lights");
	lightUniform.CopyData(&allLights, sizeof(allLights));

	lightStageUniform = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(LightStageUBOData), "Light Stage");

	WriteBufferDescriptor(device, *descriptors[Descriptors::LightState], 0, vk::DescriptorType::eUniformBuffer, lightStageUniform);
	WriteBufferDescriptor(device, *descriptors[Descriptors::Lighting], 0, vk::DescriptorType::eUniformBuffer, lightUniform);

	UpdateDescriptors();
}

void DeferredExample::OnWindowResize(uint32_t width, uint32_t height) {
	CreateFrameBuffers(width, height);
}

void	DeferredExample::LoadShaders() {
	gBufferShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("DeferredScene.vert.spv")
		.WithFragmentBinary("DeferredScene.frag.spv")
	.Build("Deferred GBuffer Fill Shader");

	lightingShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("DeferredLight.vert.spv")
		.WithFragmentBinary("DeferredLight.frag.spv")
	.Build("Deferred Lighting Shader");

	combineShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("DeferredCombine.vert.spv")
		.WithFragmentBinary("DeferredCombine.frag.spv")
	.Build("Deferred Combine Shader");
}

void	DeferredExample::CreateFrameBuffers(uint32_t width, uint32_t height) {
	TextureBuilder builder(renderer->GetDevice(), renderer->GetMemoryAllocator());

	builder.UsingPool(renderer->GetCommandPool(CommandType::Graphics))
			.UsingQueue(renderer->GetQueue(CommandType::Graphics))
			.WithDimension(width, height, 1)
			.WithMips(false)
			.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled)
			.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.WithLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.WithFormat(vk::Format::eB8G8R8A8Unorm);

	bufferTextures[ScreenTextures::Albedo]		= builder.Build("GBuffer Diffuse");
	bufferTextures[ScreenTextures::Normals]		= builder.Build("GBuffer Normals");
	bufferTextures[ScreenTextures::Diffuse]		= builder.Build("Deferred Diffuse");
	bufferTextures[ScreenTextures::Specular]	= builder.Build("Deferred Specular");

	bufferTextures[ScreenTextures::Depth] = builder
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
	.Build("GBuffer Depth");

	UpdateDescriptors();
}

void	DeferredExample::UpdateDescriptors() {
	if (!descriptors[Descriptors::LightTexture]) {
		return;
	}

	DescriptorSetWriter(renderer->GetDevice(), *descriptors[Descriptors::LightTexture])
		.WriteImage(0, *bufferTextures[1], *defaultSampler)
		.WriteImage(1, *bufferTextures[2], *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	DescriptorSetWriter(renderer->GetDevice(), *descriptors[Descriptors::Combine])
		.WriteImage(0, *bufferTextures[0], *defaultSampler)
		.WriteImage(1, *bufferTextures[3], *defaultSampler)
		.WriteImage(2, *bufferTextures[4], *defaultSampler);
}

void	DeferredExample::BuildPipelines() {
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	FrameState const& frameState = renderer->GetFrameState();
	{
		gBufferPipeline = PipelineBuilder(device)
			.WithVertexInputState(cubeMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleList)
			.WithShader(gBufferShader)
			.WithColourAttachment(bufferTextures[ScreenTextures::Albedo]->GetFormat())
			.WithColourAttachment(bufferTextures[ScreenTextures::Normals]->GetFormat())
			.WithDepthAttachment(bufferTextures[ScreenTextures::Depth]->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
			.Build("Main Scene Pipeline");

		boxObject.descriptorSet		= CreateDescriptorSet(device, pool, gBufferShader->GetLayout(1));
		floorObject.descriptorSet	= CreateDescriptorSet(device, pool, gBufferShader->GetLayout(1));

		DescriptorSetWriter(device, *boxObject.descriptorSet)
			.WriteImage(0, *objectTextures[0], *defaultSampler)
			.WriteImage(1, *objectTextures[1], *defaultSampler);

		DescriptorSetWriter(device, *floorObject.descriptorSet)
			.WriteImage(0, *objectTextures[2], *defaultSampler)
			.WriteImage(1, *objectTextures[3], *defaultSampler);
	}
	{
		lightPipeline = PipelineBuilder(device)
			.WithVertexInputState(sphereMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleList)
			.WithShader(lightingShader)
			.WithRasterState(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)

			.WithColourAttachment(bufferTextures[ScreenTextures::Diffuse]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)
			.WithColourAttachment(bufferTextures[ScreenTextures::Specular]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)

		.Build("Deferred Lighting Pipeline");

		descriptors[Descriptors::Lighting]		= CreateDescriptorSet(device, pool, lightingShader->GetLayout(1));
		descriptors[Descriptors::LightState]	= CreateDescriptorSet(device, pool, lightingShader->GetLayout(2));
		descriptors[Descriptors::LightTexture]	= CreateDescriptorSet(device, pool, lightingShader->GetLayout(3));
	}
	{
		combinePipeline = PipelineBuilder(device)
			.WithVertexInputState(quadMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
			.WithShader(combineShader)		
			.WithColourAttachment(frameState.colourFormat)
			.WithDepthAttachment(frameState.depthFormat)
		.Build("Post process pipeline");

		descriptors[Descriptors::Combine] = CreateDescriptorSet(device, pool, combineShader->GetLayout(0));
	}
}

void DeferredExample::RenderFrame(float dt) {
	LightStageUBOData newData;
	newData.camPosition = camera.GetPosition();

	Vector2i screenSize = hostWindow.GetScreenSize();

	newData.screenResolution.x = 1.0f / (float)(screenSize.x);
	newData.screenResolution.y = 1.0f / (float)(screenSize.y);
	newData.inverseViewProj = Matrix::Inverse(camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()) * camera.BuildViewMatrix());

	lightStageUniform.CopyData(&newData, sizeof(LightStageUBOData));

	FillGBuffer();
	RenderLights();
	CombineBuffers();
}

void	DeferredExample::FillGBuffer() {
	FrameState const& frameState = renderer->GetFrameState();
	frameState.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferPipeline);

	frameState.cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithColourAttachment({
				.imageView		= *bufferTextures[ScreenTextures::Albedo],
				.imageLayout	= vk::ImageLayout::eColorAttachmentOptimal,
				.loadOp			= vk::AttachmentLoadOp::eClear,
				.clearValue		= vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
			.WithColourAttachment({
				.imageView		= *bufferTextures[ScreenTextures::Normals],
				.imageLayout	= vk::ImageLayout::eColorAttachmentOptimal,
				.loadOp			= vk::AttachmentLoadOp::eClear,
				.clearValue		= vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
			.WithDepthAttachment({
				.imageView		= *bufferTextures[ScreenTextures::Depth],
				.imageLayout	= vk::ImageLayout::eDepthAttachmentOptimal,
				.loadOp			= vk::AttachmentLoadOp::eClear,
				.clearValue		= vk::ClearValue({1.0f})
			})
			.WithRenderArea(frameState.defaultScreenRect)
			.Build()
	);

	WriteBufferDescriptor(renderer->GetDevice(), *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	frameState.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, frameState.cmdBuffer, gBufferPipeline	, 1);
	RenderSingleObject(floorObject	, frameState.cmdBuffer, gBufferPipeline	, 1);

	frameState.cmdBuffer.endRendering();
}

void	DeferredExample::RenderLights() {	//Second step: Take in the GBuffer and render lights
	FrameState const& frameState = renderer->GetFrameState();
	frameState.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightPipeline);
	TransitionColourToSampler(frameState.cmdBuffer, *bufferTextures[ScreenTextures::Normals]);
	TransitionDepthToSampler(frameState.cmdBuffer, *bufferTextures[ScreenTextures::Depth]);

	frameState.cmdBuffer.beginRendering(
		DynamicRenderBuilder()
		.WithColourAttachment({
			.imageView = *bufferTextures[ScreenTextures::Diffuse],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
		.WithColourAttachment({
			.imageView = *bufferTextures[ScreenTextures::Specular],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
		.WithRenderArea(frameState.defaultScreenRect)
		.Build()
	);

	DescriptorSetMultiBinder()
		.Bind(*cameraDescriptor, 0)
		.Bind(*descriptors[Descriptors::Lighting], 1)
		.Bind(*descriptors[Descriptors::LightState], 2)
		.Bind(*descriptors[Descriptors::LightTexture], 3)
		.Commit(frameState.cmdBuffer, *lightPipeline.layout);

	sphereMesh->Draw(frameState.cmdBuffer, LIGHTCOUNT);

	frameState.cmdBuffer.endRendering();
}

void	DeferredExample::CombineBuffers() {
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	TransitionColourToSampler(cmdBuffer, *bufferTextures[ScreenTextures::Albedo]);
	TransitionColourToSampler(cmdBuffer, *bufferTextures[ScreenTextures::Diffuse]);
	TransitionColourToSampler(cmdBuffer, *bufferTextures[ScreenTextures::Specular]);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, combinePipeline);

	renderer->BeginDefaultRendering(cmdBuffer);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*descriptors[Descriptors::Combine], 0, nullptr);

	quadMesh->Draw(cmdBuffer);

	cmdBuffer.endRendering();

	TransitionSamplerToColour(cmdBuffer, *bufferTextures[ScreenTextures::Albedo]);
	TransitionSamplerToColour(cmdBuffer, *bufferTextures[ScreenTextures::Normals]);

	TransitionSamplerToDepth(cmdBuffer, *bufferTextures[ScreenTextures::Depth]);

	TransitionSamplerToColour(cmdBuffer, *bufferTextures[ScreenTextures::Diffuse]);
	TransitionSamplerToColour(cmdBuffer, *bufferTextures[ScreenTextures::Specular]);
}
//Old tutorial was 264 LOC...