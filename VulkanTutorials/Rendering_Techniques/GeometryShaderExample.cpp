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

GeometryShaderExample::GeometryShaderExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	pipeline = PipelineBuilder(context.device)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithShaderBinary("BasicGeometry.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicGeom.geom.spv", vk::ShaderStageFlagBits::eGeometry)
		.WithShaderBinary("BasicGeometry.frag.spv", vk::ShaderStageFlagBits::eFragment)
	.Build("Geometry Shader Example Pipeline");
}

void GeometryShaderExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	m_triangleMesh->Draw(context.cmdBuffer);
}