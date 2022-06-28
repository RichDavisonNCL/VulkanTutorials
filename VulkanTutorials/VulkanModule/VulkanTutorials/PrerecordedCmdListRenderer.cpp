/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PrerecordedCmdListRenderer.h"

using namespace NCL;
using namespace Rendering;

PrerecordedCmdListRenderer::PrerecordedCmdListRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(device);
	 
	BuildPipeline();

	auto buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
		commandPool, vk::CommandBufferLevel::eSecondary, 1));

	recordedBuffer = buffers[0];

	vk::CommandBufferInheritanceInfo inheritance;
	inheritance.setRenderPass(defaultRenderPass);
	vk::CommandBufferBeginInfo bufferBegin = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance);
	recordedBuffer.begin(bufferBegin);
	recordedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	recordedBuffer.setViewport(0, 1, &defaultViewport);
	recordedBuffer.setScissor(0, 1, &defaultScissor);
	SubmitDrawCall(*triMesh, recordedBuffer);
	recordedBuffer.end();
}

PrerecordedCmdListRenderer::~PrerecordedCmdListRenderer()	{

}

void PrerecordedCmdListRenderer::BuildPipeline() {
	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
	.Build(device, pipelineCache);
}

void PrerecordedCmdListRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
		.WithSecondaryBuffers()
		.Begin(defaultCmdBuffer);

	defaultCmdBuffer.executeCommands(1, &recordedBuffer);

	EndRendering(defaultCmdBuffer);
}