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

GeometryShaderExample::GeometryShaderExample(Window& window) : VulkanTutorialRenderer(window) {
}

void GeometryShaderExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceFeatures.features.setGeometryShader(true);
}

void GeometryShaderExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	mesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Geometry Shader!")
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithGeometryBinary("BasicGeom.geom.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build(GetDevice());

	pipeline = VulkanPipelineBuilder("Geometry Shader Example Pipeline")
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithShader(shader)
	.Build(GetDevice());
}

void GeometryShaderExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	SubmitDrawCall(frameCmds, *mesh);
}