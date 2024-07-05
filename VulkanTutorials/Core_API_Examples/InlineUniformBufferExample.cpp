/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "InlineUniformBufferExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(InlineUniformBufferExample)

const size_t byteCount = sizeof(Matrix4) * 2;

InlineUniformBufferExample::InlineUniformBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	static vk::PhysicalDeviceInlineUniformBlockFeatures blockFeatures;
	blockFeatures.inlineUniformBlock = true;

	vkInit.features.push_back(&blockFeatures);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
		.Build("Basic Uniform Buffer Usage!");

	inlineLayout = DescriptorSetLayoutBuilder(renderer->GetDevice())
		.WithDescriptor(vk::DescriptorType::eInlineUniformBlock, 0, byteCount, vk::ShaderStageFlagBits::eVertex)
		.Build("Inline Layout"); //New!

	camera.SetPosition({ 0,0,10 });

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithDescriptorSetLayout(0, inlineLayout) //New!
		.Build("UBO Pipeline");

	descriptorSet = CreateDescriptorSet(renderer->GetDevice(), renderer->GetDescriptorPool(), *inlineLayout); //Changed!
}

InlineUniformBufferExample::~InlineUniformBufferExample() {
}

void InlineUniformBufferExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	camera.UpdateCamera(dt);

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform(state.cmdBuffer);

	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	triMesh->Draw(state.cmdBuffer);
}

void InlineUniformBufferExample::UpdateCameraUniform(vk::CommandBuffer buffer) {
	Matrix4 camMatrices[2];
	camMatrices[0] = camera.BuildViewMatrix();
	camMatrices[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());

	vk::WriteDescriptorSetInlineUniformBlock inlineWrite;

	inlineWrite.dataSize = byteCount;
	inlineWrite.pData = &camMatrices;

	vk::WriteDescriptorSet descriptorWrite = {
		.pNext  = &inlineWrite,	//New!
		.dstSet = *descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = byteCount,
		.descriptorType = vk::DescriptorType::eInlineUniformBlock, //New!
	};

	renderer->GetDevice().updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}