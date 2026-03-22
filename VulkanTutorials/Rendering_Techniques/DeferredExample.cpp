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

DeferredExample::DeferredExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.autoBeginDynamicRendering = false;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	m_camera.SetPitch(-20.0f).SetPosition({ 0, 75.0f, 200 }).SetFarPlane(1000.0f);

	boxObject.mesh		= &*m_cubeMesh;
	boxObject.transform = Matrix::Translation(Vector3{ -50, 10, -50 }) * Matrix::Scale(Vector3{ 10.2f, 10.2f, 10.2f });

	floorObject.mesh		= &*m_cubeMesh;
	floorObject.transform = Matrix::Scale(Vector3{ 500.0f, 1.0f, 500.0f });

	objectTextures[0] = LoadTexture("rust_diffuse.png");
	objectTextures[1] = LoadTexture("rust_bump.png");
	objectTextures[2] = LoadTexture("concrete_diffuse.png");
	objectTextures[3] = LoadTexture("concrete_bump.png");

	CreateFrameBuffers(m_hostWindow.GetScreenSize().x, m_hostWindow.GetScreenSize().y);
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

	lightUniform = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(allLights),
			.usage = vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Lights"
	);
	lightUniform.CopyData(&allLights, sizeof(allLights));

	lightStageUniform = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(LightStageUBOData),
			.usage = vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Light Stage"
	);

	WriteBufferDescriptor(context.device, *descriptorSets[Descriptors::LightState], 0, vk::DescriptorType::eUniformBuffer, lightStageUniform);
	WriteBufferDescriptor(context.device, *descriptorSets[Descriptors::Lighting], 0, vk::DescriptorType::eUniformBuffer, lightUniform);

	UpdateDescriptors();
}

void DeferredExample::OnWindowResize(uint32_t width, uint32_t height) {
	CreateFrameBuffers(width, height);
}

void	DeferredExample::CreateFrameBuffers(uint32_t width, uint32_t height) {
	FrameContext const& context = m_renderer->GetFrameContext();

	vk::UniqueCommandBuffer cmdBuffer = Vulkan::CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics]);

	TextureBuilder builder(context.device, *m_memoryManager);

	builder.WithCommandBuffer(*cmdBuffer)
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

	Vulkan::CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	UpdateDescriptors();
}

void	DeferredExample::UpdateDescriptors() {
	if (!descriptorSets[Descriptors::LightTexture]) {
		return;
	}
	FrameContext const& context = m_renderer->GetFrameContext();

	DescriptorSetWriter(context.device, *descriptorSets[Descriptors::LightTexture])
		.WriteImage(0, *bufferTextures[1], *m_defaultSampler)
		.WriteImage(1, *bufferTextures[2], *m_defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	DescriptorSetWriter(context.device, *descriptorSets[Descriptors::Combine])
		.WriteImage(0, *bufferTextures[0], *m_defaultSampler)
		.WriteImage(1, *bufferTextures[3], *m_defaultSampler)
		.WriteImage(2, *bufferTextures[4], *m_defaultSampler);
}

void	DeferredExample::BuildPipelines() {
	FrameContext const& context = m_renderer->GetFrameContext();
	{
		gBufferPipeline = PipelineBuilder(context.device)
			.WithVertexInputState(m_cubeMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleList)
			.WithShaderBinary("DeferredScene.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("DeferredScene.frag.spv", vk::ShaderStageFlagBits::eFragment)
			.WithColourAttachment(bufferTextures[ScreenTextures::Albedo]->GetFormat())
			.WithColourAttachment(bufferTextures[ScreenTextures::Normals]->GetFormat())
			.WithDepthAttachment(bufferTextures[ScreenTextures::Depth]->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
			.Build("Main Scene Pipeline");

		boxObject.descriptorSet		= CreateDescriptorSet(context.device, context.descriptorPool, gBufferPipeline.GetSetLayout(1));
		floorObject.descriptorSet	= CreateDescriptorSet(context.device, context.descriptorPool, gBufferPipeline.GetSetLayout(1));

		DescriptorSetWriter(context.device, *boxObject.descriptorSet)
			.WriteImage(0, *objectTextures[0], *m_defaultSampler)
			.WriteImage(1, *objectTextures[1], *m_defaultSampler);

		DescriptorSetWriter(context.device, *floorObject.descriptorSet)
			.WriteImage(0, *objectTextures[2], *m_defaultSampler)
			.WriteImage(1, *objectTextures[3], *m_defaultSampler);
	}
	{
		lightPipeline = PipelineBuilder(context.device)
			.WithVertexInputState(m_sphereMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleList)
			.WithShaderBinary("DeferredLight.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("DeferredLight.frag.spv", vk::ShaderStageFlagBits::eFragment)
			.WithRasterState(vk::CullModeFlagBits::eFront, vk::PolygonMode::eFill)

			.WithColourAttachment(bufferTextures[ScreenTextures::Diffuse]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)
			.WithColourAttachment(bufferTextures[ScreenTextures::Specular]->GetFormat(), vk::BlendFactor::eOne, vk::BlendFactor::eOne)

		.Build("Deferred Lighting Pipeline");

		descriptorSets[Descriptors::Lighting]		= CreateDescriptorSet(context.device, context.descriptorPool, lightPipeline.GetSetLayout(1));
		descriptorSets[Descriptors::LightState]	= CreateDescriptorSet(context.device, context.descriptorPool, lightPipeline.GetSetLayout(2));
		descriptorSets[Descriptors::LightTexture]	= CreateDescriptorSet(context.device, context.descriptorPool, lightPipeline.GetSetLayout(3));
	}
	{
		combinePipeline = PipelineBuilder(context.device)
			.WithVertexInputState(m_quadMesh->GetVertexInputState())
			.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
			.WithShaderBinary("DeferredCombine.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("DeferredCombine.frag.spv", vk::ShaderStageFlagBits::eFragment)
			.WithColourAttachment(context.colourFormat)
			.WithDepthAttachment(context.depthFormat)
		.Build("Post process pipeline");

		descriptorSets[Descriptors::Combine] = CreateDescriptorSet(context.device, context.descriptorPool, combinePipeline.GetSetLayout(0));
	}
}

void DeferredExample::RenderFrame(float dt) {
	LightStageUBOData newData;
	newData.camPosition = m_camera.GetPosition();

	Vector2i screenSize = m_hostWindow.GetScreenSize();

	newData.screenResolution.x = 1.0f / (float)(screenSize.x);
	newData.screenResolution.y = 1.0f / (float)(screenSize.y);
	newData.inverseViewProj = Matrix::Inverse(m_camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect()) * m_camera.BuildViewMatrix());

	lightStageUniform.CopyData(&newData, sizeof(LightStageUBOData));

	FillGBuffer();
	RenderLights();
	CombineBuffers();
}

void	DeferredExample::FillGBuffer() {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferPipeline);

	context.cmdBuffer.beginRendering(
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
			.WithRenderArea(context.screenRect)
			.Build()
	);

	WriteBufferDescriptor(context.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *gBufferPipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, context.cmdBuffer, gBufferPipeline	, 1);
	RenderSingleObject(floorObject	, context.cmdBuffer, gBufferPipeline	, 1);

	context.cmdBuffer.endRendering();
}

void	DeferredExample::RenderLights() {	//Second step: Take in the GBuffer and render lights
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightPipeline);
	TransitionColourToSampler(context.cmdBuffer, *bufferTextures[ScreenTextures::Normals]);
	TransitionDepthToSampler(context.cmdBuffer, *bufferTextures[ScreenTextures::Depth]);

	context.cmdBuffer.beginRendering(
		DynamicRenderBuilder()
		.WithColourAttachment({
			.imageView		= *bufferTextures[ScreenTextures::Diffuse],
			.imageLayout	= vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp			= vk::AttachmentLoadOp::eClear,
			.clearValue		= vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
		.WithColourAttachment({
			.imageView		= *bufferTextures[ScreenTextures::Specular],
			.imageLayout	= vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp			= vk::AttachmentLoadOp::eClear,
			.clearValue		= vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
			})
		.WithRenderArea(context.screenRect)
		.Build()
	);

	vk::DescriptorSet sets[] = {
		*m_cameraDescriptor,
		*descriptorSets[Descriptors::Lighting],
		*descriptorSets[Descriptors::LightState],
		*descriptorSets[Descriptors::LightTexture],
	};

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *lightPipeline.layout, 0, std::size(sets), sets, 0, nullptr);

	m_sphereMesh->Draw(context.cmdBuffer, LIGHTCOUNT);

	context.cmdBuffer.endRendering();
}

void	DeferredExample::CombineBuffers() {
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer m_cmdBuffer = context.cmdBuffer;

	TransitionColourToSampler(m_cmdBuffer, *bufferTextures[ScreenTextures::Albedo]);
	TransitionColourToSampler(m_cmdBuffer, *bufferTextures[ScreenTextures::Diffuse]);
	TransitionColourToSampler(m_cmdBuffer, *bufferTextures[ScreenTextures::Specular]);

	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, combinePipeline);

	m_renderer->BeginRenderToScreen(m_cmdBuffer);

	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *combinePipeline.layout, 0, 1, &*descriptorSets[Descriptors::Combine], 0, nullptr);

	m_quadMesh->Draw(m_cmdBuffer);

	m_cmdBuffer.endRendering();

	TransitionSamplerToColour(m_cmdBuffer, *bufferTextures[ScreenTextures::Albedo]);
	TransitionSamplerToColour(m_cmdBuffer, *bufferTextures[ScreenTextures::Normals]);

	TransitionSamplerToDepth(m_cmdBuffer, *bufferTextures[ScreenTextures::Depth]);

	TransitionSamplerToColour(m_cmdBuffer, *bufferTextures[ScreenTextures::Diffuse]);
	TransitionSamplerToColour(m_cmdBuffer, *bufferTextures[ScreenTextures::Specular]);
}
//Old tutorial was 264 LOC...