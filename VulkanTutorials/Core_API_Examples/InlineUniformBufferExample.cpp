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

InlineUniformBufferExample::InlineUniformBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	static vk::PhysicalDeviceInlineUniformBlockFeatures blockFeatures;
	blockFeatures.inlineUniformBlock = true;

	m_vkInit.features.push_back(&blockFeatures);

	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	inlineLayout = DescriptorSetLayoutBuilder(context.device)
		.WithDescriptor(vk::DescriptorType::eInlineUniformBlock, 0, byteCount, vk::ShaderStageFlagBits::eVertex)
		.Build("Inline Layout"); //New!

	camera.SetPosition({ 0,0,10 });

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicUniformBuffer.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicUniformBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithDescriptorSetLayout(0, inlineLayout) //New!
		.Build("UBO Pipeline");

	descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, *inlineLayout); //Changed!
}

void InlineUniformBufferExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	camera.UpdateCamera(dt);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform(context.cmdBuffer);

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	m_triangleMesh->Draw(context.cmdBuffer);
}

void InlineUniformBufferExample::UpdateCameraUniform(vk::CommandBuffer buffer) {
	FrameContext const& context = m_renderer->GetFrameContext();

	Matrix4 camMatrices[2] = {
		camera.BuildViewMatrix(),
		camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect())
	};

	vk::WriteDescriptorSetInlineUniformBlock inlineWrite = {
		.dataSize	= byteCount,
		.pData		= &camMatrices
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.pNext				= &inlineWrite,	//New!
		.dstSet				= *descriptorSet,
		.dstBinding			= 0,
		.dstArrayElement	= 0,
		.descriptorCount	= byteCount, //Changed!
		.descriptorType		= vk::DescriptorType::eInlineUniformBlock, //New!
	};

	context.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}