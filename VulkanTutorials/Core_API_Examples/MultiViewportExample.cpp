/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "MultiViewportExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(MultiViewportExample)

MultiViewportExample::MultiViewportExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithShaderBinary("BasicGeometry.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicGeometry.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.Build("Multi Viewport Pipeline");
}

void MultiViewportExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	Vector2i windowSize = m_hostWindow.GetScreenSize();

	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = ((float)windowSize.x / 2.0f);
		float ySize = ((float)windowSize.y / 2.0f);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		context.cmdBuffer.setViewport(0, 1, &viewport);
		m_triangleMesh->Draw(context.cmdBuffer);
	}
}