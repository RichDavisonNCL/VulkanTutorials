/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "DescriptorBufferExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(DescriptorBufferExample)

DescriptorBufferExample::DescriptorBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	m_vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);

	m_vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	static vk::PhysicalDeviceDescriptorBufferFeaturesEXT	descriptorBufferfeature = {
		.descriptorBuffer = true
	};
	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
	};

	m_vkInit.features.push_back((void*)&descriptorBufferfeature);
	m_vkInit.features.push_back((void*)&deviceAddressfeature);

	Initialise();

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &descriptorProperties;

	m_renderer->GetPhysicalDevice().getProperties2(&props);

	FrameContext const& context = m_renderer->GetFrameContext();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicDescriptor.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicDescriptor.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDescriptorSetLayoutCreationFlags(0, vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithCreationFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT) //New!
		.Build("Descriptor Buffer Pipeline");

	context.device.getDescriptorSetLayoutSizeEXT(pipeline.m_allLayouts[0], &descriptorBufferSize);

	descriptorBuffer = m_memoryManager->CreateBuffer(
		{
				.size = descriptorBufferSize,
				.usage	= vk::BufferUsageFlagBits::eShaderDeviceAddress 
						| vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT 
						| vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Descriptor Buffer"
	);

	uniformData[0] = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(positionUniform)	,
			.usage	= vk::BufferUsageFlagBits::eShaderDeviceAddress 
					| vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Position Uniform"
	);

	uniformData[1] = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(colourUniform),
			.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress 
						| vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Colour Uniform"
	);

	//DescriptorBufferWriter(context.device, pipeline.m_allLayouts[0], descriptorBuffer, descriptorProperties)
	//	.WriteBuffer(0, vk::DescriptorType::eUniformBuffer, uniformData[0])
	//	.WriteBuffer(1, vk::DescriptorType::eUniformBuffer, uniformData[1])
	//	.Finish();
}

void DescriptorBufferExample::RenderFrame(float dt) {
	Vector3 positionUniform = Vector3(sin(m_runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(m_runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	FrameContext const& context = m_renderer->GetFrameContext();

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferBindingInfoEXT bindingInfo;
	bindingInfo.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress 
							| vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT 
							| vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
	bindingInfo.address		= descriptorBuffer.deviceAddress;

	context.cmdBuffer.bindDescriptorBuffersEXT(1, &bindingInfo);

	uint32_t indices[1] = {
		0 //Which buffers from the above binding are each set being taken from?
	};
	VkDeviceSize offsets[1] = {
		0 //How many bytes into the bound buffer is each descriptor set?
	};

	context.cmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,	//First descriptor set
		1,	//Number of descriptor sets 
		indices,
		offsets);

	m_triangleMesh->Draw(context.cmdBuffer);
}