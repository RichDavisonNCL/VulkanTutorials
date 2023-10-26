/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicGeometryRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

BasicGeometryRenderer::BasicGeometryRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

void BasicGeometryRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Basic Shader!");

	basicPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithShader(shader)
	.Build("Basic Pipeline");
}

void BasicGeometryRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	DrawMesh(frameCmds, *triMesh);
}