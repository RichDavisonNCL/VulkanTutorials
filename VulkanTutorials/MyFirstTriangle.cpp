/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "MyFirstTriangle.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

MyFirstTriangle::MyFirstTriangle(Window& window) : VulkanTutorial(window)	{
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
	.Build("Basic Shader!");

	FrameState const& frameState = renderer->GetFrameState();

	basicPipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithShader(shader)
	.Build("Basic Pipeline");
}

void MyFirstTriangle::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	triMesh->Draw(state.cmdBuffer);
}