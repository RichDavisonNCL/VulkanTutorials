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
}

PrerecordedCmdListRenderer::~PrerecordedCmdListRenderer()	{
}

void PrerecordedCmdListRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(GetDevice());
	 
	BuildPipeline();

	auto buffers = GetDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo(
		GetCommandPool(CommandBufferType::Graphics), vk::CommandBufferLevel::eSecondary, 1));

	recordedBuffer = buffers[0];

	vk::CommandBufferInheritanceInfo inheritance;
	inheritance.setRenderPass(defaultRenderPass);
	vk::CommandBufferBeginInfo bufferBegin = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance);
	recordedBuffer.begin(bufferBegin);
	recordedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	recordedBuffer.setViewport(0, 1, &defaultViewport);
	recordedBuffer.setScissor(0, 1, &defaultScissor);
	SubmitDrawCall(recordedBuffer, *triMesh);
	recordedBuffer.end();
}

void PrerecordedCmdListRenderer::BuildPipeline() {
	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build(GetDevice());
}

void PrerecordedCmdListRenderer::RenderFrame() {
	//VulkanDynamicRenderBuilder()
	//	.WithColourAttachment(swapChainList[currentSwap]->view)
	//	.WithRenderArea(defaultScreenRect)
	//	.WithSecondaryBuffers()
	//	.BeginRendering(defaultCmdBuffer);

	frameCmds.executeCommands(1, &recordedBuffer);

	//EndRendering(defaultCmdBuffer);
}