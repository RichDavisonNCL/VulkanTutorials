/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PrerecordedCmdListRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

PrerecordedCmdListRenderer::PrerecordedCmdListRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

PrerecordedCmdListRenderer::~PrerecordedCmdListRenderer()	{
}

void PrerecordedCmdListRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Basic Shader!");
	 
	BuildPipeline();

	auto buffers = GetDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo(
		GetCommandPool(CommandBuffer::Graphics), vk::CommandBufferLevel::eSecondary, 1));

	recordedBuffer = buffers[0];

	vk::CommandBufferInheritanceInfo inheritance;
	inheritance.setRenderPass(defaultRenderPass);
	vk::CommandBufferBeginInfo bufferBegin = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance);
	recordedBuffer.begin(bufferBegin);
	recordedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	recordedBuffer.setViewport(0, 1, &defaultViewport);
	recordedBuffer.setScissor(0, 1, &defaultScissor);
	DrawMesh(recordedBuffer, *triMesh);
	recordedBuffer.end();
}

void PrerecordedCmdListRenderer::BuildPipeline() {
	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		//.WithDepthFormat(depthBuffer->GetFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build("Prerecorded cmd list pipeline");
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