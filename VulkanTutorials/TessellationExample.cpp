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

TessellationExample::TessellationExample(Window& window) : VulkanTutorialRenderer(window) {
}

void TessellationExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceFeatures.features.setTessellationShader(true);
	deviceFeatures.features.setFillModeNonSolid(true);
}

void TessellationExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	mesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithTessControlBinary("BasicTCS.tesc.spv")
		.WithTessEvalBinary("BasicTES.tese.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Tessellation Shader!");

	pipeline = PipelineBuilder(GetDevice())
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePatchList)
		.WithRaster({}, vk::PolygonMode::eLine)
		.WithShader(shader)
		.WithTessellationPatchVertexCount(3)
		.WithPushConstant(vk::ShaderStageFlagBits::eTessellationControl, 0, sizeof(Vector4))
	.Build("Tessellation Shader Example Pipeline");
}

void TessellationExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	Vector4 tessValues{ 8,8,8,8 };

	frameCmds.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eTessellationControl, 0, sizeof(Vector4), &tessValues);

	DrawMesh(frameCmds, *mesh);
}