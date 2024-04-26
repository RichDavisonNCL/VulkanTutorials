/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PushConstantExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

PushConstantExample::PushConstantExample(Window& window) : VulkanTutorial(window)	{
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("BasicPushConstant.frag.spv")
		.Build("Testing push constants!");

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.Build("Basic Push Constant Pipeline");
}

void PushConstantExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	positionUniform = {sin(runTime), 0.0f, 0.0f };
	colourUniform	= Vector4(1.0f, 1.0f, 0.0f, 1.0f);

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	state.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform), (void*)&positionUniform);
	state.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform), (void*)&colourUniform);

	triMesh->Draw(state.cmdBuffer);
}