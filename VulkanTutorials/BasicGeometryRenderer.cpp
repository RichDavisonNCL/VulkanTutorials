/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicGeometryRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicGeometryRenderer::BasicGeometryRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

void BasicGeometryRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(device);

	basicPipeline = VulkanPipelineBuilder("Basic Pipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
	.Build(device);
}

void BasicGeometryRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *basicPipeline.pipeline);
	SubmitDrawCall(*triMesh, defaultCmdBuffer);
	EndRendering(defaultCmdBuffer);
}