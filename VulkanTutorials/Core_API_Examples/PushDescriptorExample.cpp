/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PushDescriptorExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(PushDescriptorExample)

PushDescriptorExample::PushDescriptorExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.deviceExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

	Initialise();

	vk::PhysicalDeviceProperties2 physicalProps;
	vk::PhysicalDevicePushDescriptorPropertiesKHR pushProps;
	physicalProps.pNext = &pushProps;
	m_renderer->GetPhysicalDevice().getProperties2(&physicalProps);
	std::cout << __FUNCTION__ << ": Physical device supports " << pushProps.maxPushDescriptors << " push descriptors\n";

	FrameContext const& context = m_renderer->GetFrameContext();

	uniformData[0] = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(positionUniform),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Position Uniform"
	);

	uniformData[1] = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(colourUniform),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Colour Uniform"
	);

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicDescriptor.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicDescriptor.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDescriptorSetLayoutCreationFlags(0, vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
	.Build("Descriptor Buffer Pipeline");
}

void PushDescriptorExample::RenderFrame(float dt) {
	Vector3 positionUniform = Vector3(sin(m_runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(m_runTime), 0, 1, 1);

	FrameContext const& context = m_renderer->GetFrameContext();

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferInfo bufferInfo[2] = {
		{ uniformData[0].buffer, 0, VK_WHOLE_SIZE },
		{ uniformData[1].buffer, 0, VK_WHOLE_SIZE }
	};

	vk::WriteDescriptorSet pushSets[2] = {
		{
			.dstBinding			= 0,
			.descriptorCount	= 1,
			.descriptorType		= vk::DescriptorType::eUniformBuffer,
			.pBufferInfo		= &bufferInfo[0]
		}
		,
		{
			.dstBinding			= 1,
			.descriptorCount	= 1,
			.descriptorType		= vk::DescriptorType::eUniformBuffer,
			.pBufferInfo		= &bufferInfo[1]
		}
	};

	context.cmdBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, std::size(pushSets), pushSets);

	m_triangleMesh->Draw(context.cmdBuffer);
}