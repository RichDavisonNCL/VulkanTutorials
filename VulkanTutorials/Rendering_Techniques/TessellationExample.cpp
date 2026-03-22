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

TUTORIAL_ENTRY(TessellationExample)

TessellationExample::TessellationExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	Initialise();
//	deviceFeatures.features.setTessellationShader(true);
//	deviceFeatures.features.setFillModeNonSolid(true);

	mesh = GenerateTriangle();

	FrameContext const& context = m_renderer->GetFrameContext();

	pipeline = PipelineBuilder(context.device)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePatchList)
		.WithRasterState({}, vk::PolygonMode::eLine)
		.WithShaderBinary("BasicGeometry.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicTCS.tesc.spv", vk::ShaderStageFlagBits::eTessellationControl)
		.WithShaderBinary("BasicTES.tese.spv", vk::ShaderStageFlagBits::eTessellationEvaluation)
		.WithShaderBinary("BasicGeometry.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithTessellationPatchVertexCount(3)
	.Build("Tessellation Shader Example Pipeline");
}

void TessellationExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	Vector4 tessValues{ 8,8,8,8 };

	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eTessellationControl, 0, sizeof(Vector4), &tessValues);

	mesh->Draw(context.cmdBuffer);
}