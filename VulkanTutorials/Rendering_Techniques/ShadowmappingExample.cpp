/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "ShadowmappingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(ShadowMappingExample)

const int SHADOWSIZE = 2048;

ShadowMappingExample::ShadowMappingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
	vkInit.autoBeginDynamicRendering = false;
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool  = renderer->GetDescriptorPool();
	FrameState const& frameState = renderer->GetFrameState();

	camera.SetPitch(-45.0f).SetYaw(310).SetPosition({ -50, 60.0f, 50 });

	cubeMesh	= LoadMesh("Cube.msh");
	
	shadowFillShader = ShaderBuilder(device)
		.WithVertexBinary("ShadowFill.vert.spv")
		.WithFragmentBinary("ShadowFill.frag.spv")
	.Build("Shadow Fill Shader");

	shadowUseShader = ShaderBuilder(device)
		.WithVertexBinary("ShadowUse.vert.spv")
		.WithFragmentBinary("ShadowUse.frag.spv")
	.Build("Shadow Use Shader");

	shadowMap = TextureBuilder(device, renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandType::Graphics))
		.UsingQueue(renderer->GetQueue(CommandType::Graphics))
		.WithDimension(SHADOWSIZE, SHADOWSIZE)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithMips(false)
	.Build("Shadowmap");

	sceneShadowTexDescriptor	= CreateDescriptorSet(renderer->GetDevice(), pool, shadowUseShader->GetLayout(3));
	WriteImageDescriptor(device, *sceneShadowTexDescriptor, 0, shadowMap->GetDefaultView(), *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	shadowMatBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4), "Shadow Matrix");

	shadowMatrix = (Matrix4*)shadowMatBuffer.Map(); 
	*shadowMatrix = 	//Let's make the shadow casting light look at something!
		Matrix::Perspective(1.0f, 200.0f, 1.0f, 45.0f) * 
		Matrix::View(Vector3(-50, 50, -50), Vector3(0, 0, 0), Vector3(0, 1, 0));
	shadowMatBuffer.Unmap();

	shadowMatrixDescriptor = CreateDescriptorSet(device, pool, shadowUseShader->GetLayout(1));
	WriteBufferDescriptor(device, *shadowMatrixDescriptor, 0, vk::DescriptorType::eUniformBuffer, shadowMatBuffer);

	shadowPipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowFillShader)
		.WithDepthAttachment(shadowMap->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
	.Build("Shadow Map Fill Pipeline");

	scenePipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowUseShader)
		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithColourAttachment(frameState.colourFormat)
	.Build("Main Scene Draw Pipeline");

	sceneObjects[0].mesh = &*cubeMesh;
	sceneObjects[0].transform = Matrix::Translation(Vector3{ -20, 25, -20 });
	sceneObjects[0].descriptorSet = CreateDescriptorSet(renderer->GetDevice(), pool, shadowUseShader->GetLayout(2));

	sceneObjects[1].mesh		= &*cubeMesh;
	sceneObjects[1].transform = Matrix::Scale(Vector3{ 40.0f, 1.0f, 40.0f });
	sceneObjects[1].descriptorSet = CreateDescriptorSet(renderer->GetDevice(), pool, shadowUseShader->GetLayout(2));
}

void ShadowMappingExample::RenderFrame(float dt) {
	sceneObjects[0].transform = Matrix::Translation(Vector3{-20, 10.0f + (sin(runTime) * 10.0f), -20});
	RenderShadowMap();	
	RenderScene();	
}

void ShadowMappingExample::DrawObjects(vk::CommandBuffer buffer, VulkanPipeline& pipeline) {
	for (auto& o : sceneObjects) {
		buffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&o.transform);
		o.mesh->Draw(buffer);
	}
}

void ShadowMappingExample::RenderShadowMap() {
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline);

	vk::Viewport shadowViewport = vk::Viewport(0.0f, (float)SHADOWSIZE, (float)SHADOWSIZE, -(float)SHADOWSIZE, 0.0f, 1.0f);
	vk::Rect2D	 shadowScissor	= vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(SHADOWSIZE, SHADOWSIZE));

	cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithDepthAttachment(shadowMap->GetDefaultView())
			.WithRenderArea(shadowScissor)
			.Build()
	);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *shadowPipeline.layout, 0, 1, &*shadowMatrixDescriptor, 0, nullptr);
	cmdBuffer.setViewport(0, 1, &shadowViewport);
	cmdBuffer.setScissor(0, 1, &shadowScissor);

	DrawObjects(cmdBuffer, shadowPipeline);
	cmdBuffer.endRendering();
}

void ShadowMappingExample::RenderScene() {
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	TransitionDepthToSampler(cmdBuffer, *shadowMap);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, scenePipeline);
	renderer->BeginDefaultRendering(cmdBuffer);

	cmdBuffer.setViewport(0, 1, &frameState.defaultViewport);
	cmdBuffer.setScissor(0, 1, &frameState.defaultScissor);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 1, 1, &*shadowMatrixDescriptor, 0, nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 3, 1, &*sceneShadowTexDescriptor, 0, nullptr);

	DrawObjects(cmdBuffer, scenePipeline);

	cmdBuffer.endRendering();
	TransitionSamplerToDepth(cmdBuffer, *shadowMap);
}