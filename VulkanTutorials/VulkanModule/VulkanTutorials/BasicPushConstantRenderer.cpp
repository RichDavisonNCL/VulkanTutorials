/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicPushConstantRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicPushConstantRenderer::BasicPushConstantRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	positionUniform = Vector3(0.0f, 0.0f, 0.0f);
	colourUniform	= Vector4(1.0f, 1.0f, 0.0f, 1.0f);

	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Testing push constants!")
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("BasicPushConstant.frag.spv")
	.BuildUnique(device);

	pipeline = VulkanPipelineBuilder("Basic Push Constant Pipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform))
		.WithPushConstant(vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform))
	.Build(device, pipelineCache);
}

void BasicPushConstantRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	positionUniform.x = sin(runTime);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);

	defaultCmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform), (void*)&positionUniform);
	defaultCmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform), (void*)&colourUniform);

	SubmitDrawCall(*triMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}