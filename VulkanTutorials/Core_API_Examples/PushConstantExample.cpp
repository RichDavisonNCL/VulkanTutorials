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

TUTORIAL_ENTRY(PushConstantExample)

PushConstantExample::PushConstantExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicPushConstant.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicPushConstant.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.Build("Basic Push Constant Pipeline");
}

void PushConstantExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	positionUniform = {sin(m_runTime), 0.0f, 0.0f };
	colourUniform	= Vector4(1.0f, 1.0f, 0.0f, 1.0f);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform), (void*)&positionUniform);
	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform), (void*)&colourUniform);

	m_triangleMesh->Draw(context.cmdBuffer);
}