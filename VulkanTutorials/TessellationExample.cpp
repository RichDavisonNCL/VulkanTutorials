/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "TessellationExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TessellationExample::TessellationExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();
//	deviceFeatures.features.setTessellationShader(true);
//	deviceFeatures.features.setFillModeNonSolid(true);

	mesh = GenerateTriangle();

	FrameState const& frameState = renderer->GetFrameState();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithTessControlBinary("BasicTCS.tesc.spv")
		.WithTessEvalBinary("BasicTES.tese.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Tessellation Shader!");

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePatchList)
		.WithRasterState({}, vk::PolygonMode::eLine)
		.WithShader(shader)
		.WithTessellationPatchVertexCount(3)
	.Build("Tessellation Shader Example Pipeline");
}

void TessellationExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	Vector4 tessValues{ 8,8,8,8 };

	state.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eTessellationControl, 0, sizeof(Vector4), &tessValues);

	mesh->Draw(state.cmdBuffer);
}