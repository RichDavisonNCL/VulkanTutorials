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

MultiViewportExample::MultiViewportExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
		.Build("Basic Shader!");

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithShader(shader)
		.Build("Multi Viewport Pipeline");
}

void MultiViewportExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	Vector2i windowSize = hostWindow.GetScreenSize();

	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = ((float)windowSize.x / 2.0f);
		float ySize = ((float)windowSize.y / 2.0f);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		state.cmdBuffer.setViewport(0, 1, &viewport);
		triMesh->Draw(state.cmdBuffer);
	}
}