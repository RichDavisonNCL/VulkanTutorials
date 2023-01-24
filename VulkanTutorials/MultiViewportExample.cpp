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
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(device);

	pipeline = VulkanPipelineBuilder("Test Pipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
	.Build(device);
}

void MultiViewportExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = (float)(windowWidth / 2);
		float ySize = (float)(windowHeight / 2);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		defaultCmdBuffer.setViewport(0, 1, &viewport);
		SubmitDrawCall(*triMesh, defaultCmdBuffer);
	}

	EndRendering(defaultCmdBuffer);
}