/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "GeometryShaderExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(GeometryShaderExample)

GeometryShaderExample::GeometryShaderExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	mesh = GenerateTriangle();

	FrameState const& frameState = renderer->GetFrameState();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithGeometryBinary("BasicGeom.geom.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Geometry Shader!");

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithShader(shader)
	.Build("Geometry Shader Example Pipeline");
}

void GeometryShaderExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	mesh->Draw(state.cmdBuffer);
}