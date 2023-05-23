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
	.Build(GetDevice());

	basicPipeline = VulkanPipelineBuilder("Basic Pipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		//.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build(GetDevice());
}

void BasicGeometryRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	SubmitDrawCall(frameCmds, *triMesh);
}