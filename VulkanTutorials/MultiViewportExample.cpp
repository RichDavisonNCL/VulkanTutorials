/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "MultiViewportExample.h"

using namespace NCL;
using namespace Rendering;

MultiViewportExample::MultiViewportExample(Window& window) : VulkanTutorialRenderer(window) {

}

void MultiViewportExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(GetDevice());

	pipeline = VulkanPipelineBuilder("Test Pipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build(GetDevice());
}

void MultiViewportExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = (float)(windowWidth / 2);
		float ySize = (float)(windowHeight / 2);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		frameCmds.setViewport(0, 1, &viewport);
		SubmitDrawCall(frameCmds, *triMesh);
	}
}