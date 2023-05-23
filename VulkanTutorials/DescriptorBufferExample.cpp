/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "DescriptorBufferExample.h"

using namespace NCL;
using namespace Rendering;

DescriptorBufferExample::DescriptorBufferExample(Window& window) : VulkanTutorialRenderer(window) {
}

void DescriptorBufferExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	static vk::PhysicalDeviceDescriptorBufferFeaturesEXT	descriptorBufferfeature(true);
	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature(true);

	deviceAddressfeature.bufferDeviceAddress = true;

	deviceFeatures.pNext = (void*)&descriptorBufferfeature;
	descriptorBufferfeature.pNext = (void*)&deviceAddressfeature;	
	
	deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);	
	
	allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
}

void DescriptorBufferExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &descriptorProperties;

	GetPhysicalDevice().getProperties2(&props, *Vulkan::dispatcher);

	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Descriptor Shader!")
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build(GetDevice());

	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Uniform Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)	//New!
		.Build(GetDevice());

	GetDevice().getDescriptorSetLayoutSizeEXT(*descriptorLayout, &descriptorBufferSize, *Vulkan::dispatcher);

	descriptorBuffer = VulkanBufferBuilder(descriptorBufferSize, "Descriptor Buffer")
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	uniformData[0] = VulkanBufferBuilder(sizeof(positionUniform), "Position Uniform")
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	uniformData[1] = VulkanBufferBuilder(sizeof(colourUniform), "Colour Uniform")
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	uniformDataAddresses[0] = GetDevice().getBufferAddress(vk::BufferDeviceAddressInfo(uniformData[0].buffer));
	uniformDataAddresses[1] = GetDevice().getBufferAddress(vk::BufferDeviceAddressInfo(uniformData[1].buffer));

	vk::DescriptorAddressInfoEXT testAddresses[2] = {
		vk::DescriptorAddressInfoEXT(uniformDataAddresses[0], uniformData[0].size), //make this 12 to break everything!
		vk::DescriptorAddressInfoEXT(uniformDataAddresses[1], uniformData[1].size)
	};

	vk::DescriptorGetInfoEXT getInfos[2] = {
		vk::DescriptorGetInfoEXT(vk::DescriptorType::eUniformBuffer, &testAddresses[0]),
		vk::DescriptorGetInfoEXT(vk::DescriptorType::eUniformBuffer, &testAddresses[1])
	};	
	
	vk::DeviceSize		offsets[2] = {
		GetDevice().getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 0, *Vulkan::dispatcher),
		GetDevice().getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 1, *Vulkan::dispatcher)
	};
	void* descriptorBufferMemory = descriptorBuffer.Map();
	GetDevice().getDescriptorEXT(&getInfos[0], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[0], *Vulkan::dispatcher);
	GetDevice().getDescriptorEXT(&getInfos[1], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[1], *Vulkan::dispatcher);
	descriptorBuffer.Unmap();

	descriptorBufferAddress = GetDevice().getBufferAddress(vk::BufferDeviceAddressInfo(descriptorBuffer.buffer));

	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorBuffers() //New!
	.Build(GetDevice());
}

void DescriptorBufferExample::RenderFrame() {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferBindingInfoEXT bindingInfo;
	bindingInfo.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
	bindingInfo.address		= descriptorBufferAddress;

	frameCmds.bindDescriptorBuffersEXT(1, &bindingInfo, *Vulkan::dispatcher);

	uint32_t indices[1] = {
		0 //Which buffers are each set being taken from?
	};
	VkDeviceSize offsets[1] = {
		0 //How many bytes into the bound buffer is each descriptor set?
	};

	frameCmds.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,	//First descriptor set
		1,	//Number of descriptor sets 
		indices,
		offsets,
		*Vulkan::dispatcher);

	SubmitDrawCall(frameCmds, *triMesh);
}