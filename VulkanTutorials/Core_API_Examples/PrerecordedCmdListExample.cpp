/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PrerecordedCmdListExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

//TUTORIAL_ENTRY(PrerecordedCmdListExample)

//PrerecordedCmdListExample::PrerecordedCmdListExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
//}
//
//PrerecordedCmdListExample::~PrerecordedCmdListExample()	{
//}
//
//void PrerecordedCmdListExample::SetupTutorial() {
//	VulkanTutorial::SetupTutorial();
//	triMesh = GenerateTriangle();
//
//	shader = ShaderBuilder(renderer->GetDevice())
//		.WithVertexBinary("BasicGeometry.vert.spv")
//		.WithFragmentBinary("BasicGeometry.frag.spv")
//	.Build("Basic Shader!");
//	 
//	BuildPipeline();
//
//	auto buffers = renderer->GetDevice().allocateCommandBuffers(
//		{
//			.commandPool = renderer->GetCommandPool(CommandBuffer::Graphics),
//			.level = vk::CommandBufferLevel::eSecondary,
//			.commandBufferCount = 1
//		}
//	);	
//	recordedBuffer = buffers[0];
//
//	vk::CommandBufferInheritanceInfo inheritance;
//	inheritance.setRenderPass(defaultRenderPass);
//	vk::CommandBufferBeginInfo bufferBegin = {
//		.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue,
//		.pInheritanceInfo = &inheritance
//	};
//		
//	recordedBuffer.begin(bufferBegin);
//	recordedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
//	recordedBuffer.setViewport(0, 1, &defaultViewport);
//	recordedBuffer.setScissor(0, 1, &defaultScissor);
//	triMesh->Draw(recordedBuffer);
//	recordedBuffer.end();
//}
//
//void PrerecordedCmdListExample::BuildPipeline() {
//	pipeline = PipelineBuilder(renderer->GetDevice())
//		.WithVertexInputState(triMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleList)
//		//.WithDepthFormat(depthBuffer->GetFormat())
//		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat())
//		.WithShader(shader)
//	.Build("Prerecorded cmd list pipeline");
//}
//
//void PrerecordedCmdListExample::RenderFrame() {
//	//VulkanDynamicRenderBuilder()
//	//	.WithColourAttachment(swapChainList[currentSwap]->view)
//	//	.WithRenderArea(defaultScreenRect)
//	//	.WithSecondaryBuffers()
//	//	.BeginRendering(defaultCmdBuffer);
//
//	frameCmds.executeCommands(1, &recordedBuffer);
//
//	//EndRendering(defaultCmdBuffer);
//}