/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicPushConstantRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

BasicPushConstantRenderer::BasicPushConstantRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

void BasicPushConstantRenderer::SetupTutorial()	{
	VulkanTutorialRenderer::SetupTutorial();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("BasicPushConstant.frag.spv")
	.Build("Testing push constants!");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform))
		.WithPushConstant(vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform))
	.Build("Basic Push Constant Pipeline");
}

void BasicPushConstantRenderer::RenderFrame() {
	positionUniform = {sin(runTime), 0.0f, 0.0f };
	colourUniform	= Vector4(1.0f, 1.0f, 0.0f, 1.0f);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	frameCmds.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform), (void*)&positionUniform);
	frameCmds.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform), (void*)&colourUniform);

	DrawMesh(frameCmds, *triMesh);
}