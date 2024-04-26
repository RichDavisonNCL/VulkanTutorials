/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "MultiPipelineExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

MultiPipelineExample::MultiPipelineExample(Window& window) : VulkanTutorial(window)	{
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("doge.png");

	triangleMesh = GenerateTriangle();
	triangleMesh = GenerateQuad();

	solidColourShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SolidColour.frag.spv")
	.Build("Solid Colour Shader");

	texturingShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	FrameState const& frameState = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	solidColourPipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
		.WithDepthAttachment(frameState.depthFormat)
		.WithShader(solidColourShader)
	.Build("Solid colour Pipeline!");

	texturedPipeline = PipelineBuilder(device)
		.WithVertexInputState(triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
		.WithDepthAttachment(frameState.depthFormat)
		.WithShader(texturingShader)
		.Build("Texturing Pipeline");

	descriptorSets[0] = CreateDescriptorSet(device, pool, texturingShader->GetLayout(1));
	descriptorSets[1] = CreateDescriptorSet(device, pool, texturingShader->GetLayout(1));
	WriteImageDescriptor(device, *descriptorSets[0], 0, textures[0]->GetDefaultView(), *defaultSampler);
	WriteImageDescriptor(device, *descriptorSets[1], 0, textures[1]->GetDefaultView(), *defaultSampler);

}

void MultiPipelineExample::RenderFrame(float dt) {
	Vector4 objectPositions[3] = { Vector4(0.5, 0, 0, 1), Vector4(-0.5, 0, 0, 1), Vector4(0, 0.5, 0, 1) };

	FrameState const& state = renderer->GetFrameState();

	//First draw the solid colour triangle
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, solidColourPipeline);
	state.cmdBuffer.pushConstants(*solidColourPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[0]);

	triangleMesh->Draw(state.cmdBuffer);

	////Now to draw the two textured triangles!
	////They both use the same pipeline, but different descriptors, as they have different textures
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, texturedPipeline);
	state.cmdBuffer.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[1]);
	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &*descriptorSets[0], 0, nullptr);
	triangleMesh->Draw(state.cmdBuffer);

	state.cmdBuffer.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[2]);
	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &*descriptorSets[1], 0, nullptr);
	triangleMesh->Draw(state.cmdBuffer);
}