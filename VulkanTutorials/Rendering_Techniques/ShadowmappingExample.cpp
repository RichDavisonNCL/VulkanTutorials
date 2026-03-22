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

ShadowMappingExample::ShadowMappingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.autoBeginDynamicRendering = false;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	m_camera.SetPitch(-45.0f).SetYaw(310).SetPosition({ -50, 60.0f, 50 });

	shadowMap = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(context.cmdBuffer)
		.WithDimension(SHADOWSIZE, SHADOWSIZE)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithMips(false)
	.Build("Shadowmap");

	shadowMatBuffer = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(Matrix4),
			.usage = vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Shadow Matrix"
	);

	shadowMatrix = (Matrix4*)shadowMatBuffer.Map(); 
	*shadowMatrix = 	//Let's make the shadow casting light look at something!
		Matrix::Perspective(1.0f, 200.0f, 1.0f, 45.0f) * 
		Matrix::View(Vector3(-50, 50, -50), Vector3(0, 0, 0), Vector3(0, 1, 0));
	shadowMatBuffer.Unmap();

	shadowPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("ShadowFill.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("ShadowFill.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDepthAttachment(shadowMap->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
	.Build("Shadow Map Fill Pipeline");

	scenePipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("ShadowUse.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("ShadowUse.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.WithColourAttachment(context.colourFormat)
	.Build("Main Scene Draw Pipeline");

	sceneObjects[0].mesh = &*m_cubeMesh;
	sceneObjects[0].transform = Matrix::Translation(Vector3{ -20, 25, -20 });
	sceneObjects[0].descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(2));

	sceneObjects[1].mesh		= &*m_cubeMesh;
	sceneObjects[1].transform = Matrix::Scale(Vector3{ 40.0f, 1.0f, 40.0f });
	sceneObjects[1].descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(2));

	sceneShadowTexDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(3));
	WriteCombinedImageDescriptor(context.device, *sceneShadowTexDescriptor, 0, shadowMap->GetDefaultView(), *m_defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	shadowMatrixDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(1));
	WriteBufferDescriptor(context.device, *shadowMatrixDescriptor, 0, vk::DescriptorType::eUniformBuffer, shadowMatBuffer);
}

void ShadowMappingExample::RenderFrame(float dt) {
	sceneObjects[0].transform = Matrix::Translation(Vector3{-20, 10.0f + (sin(m_runTime) * 10.0f), -20});
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
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer m_cmdBuffer = context.cmdBuffer;

	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline);

	DynamicRenderBuilder()
		.WithDepthAttachment(shadowMap->GetDefaultView())
		.WithRenderArea(vk::Offset2D(0, 0), vk::Extent2D(SHADOWSIZE, SHADOWSIZE))
		.BeginRendering(m_cmdBuffer);

	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *shadowPipeline.layout, 0, 1, &*shadowMatrixDescriptor, 0, nullptr);

	DrawObjects(m_cmdBuffer, shadowPipeline);
	m_cmdBuffer.endRendering();
}

void ShadowMappingExample::RenderScene() {
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer m_cmdBuffer = context.cmdBuffer;

	TransitionDepthToSampler(m_cmdBuffer, *shadowMap);

	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, scenePipeline);
	m_renderer->BeginRenderToScreen(m_cmdBuffer);

	m_cmdBuffer.setViewport(0, 1, &context.viewport);
	m_cmdBuffer.setScissor(0, 1, &context.screenRect);

	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);
	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 1, 1, &*shadowMatrixDescriptor, 0, nullptr);
	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 3, 1, &*sceneShadowTexDescriptor, 0, nullptr);

	DrawObjects(m_cmdBuffer, scenePipeline);

	m_cmdBuffer.endRendering();
	TransitionSamplerToDepth(m_cmdBuffer, *shadowMap);
}