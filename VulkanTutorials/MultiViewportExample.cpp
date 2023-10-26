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

MultiViewportExample::MultiViewportExample(Window& window) : VulkanTutorialRenderer(window) {
}

void MultiViewportExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Basic Shader!");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build("Multi Viewport Pipeline");
}

void MultiViewportExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = ((float)windowSize.x / 2.0f);
		float ySize = ((float)windowSize.y / 2.0f);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		frameCmds.setViewport(0, 1, &viewport);
		DrawMesh(frameCmds, *triMesh);
	}
}