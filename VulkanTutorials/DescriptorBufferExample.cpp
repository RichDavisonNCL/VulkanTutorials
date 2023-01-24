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
	deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
}

void DescriptorBufferExample::SetupDeviceInfo(vk::DeviceCreateInfo& createInfo) {
	static vk::PhysicalDeviceDescriptorBufferFeaturesEXT	descriptorBufferfeature(true);
	static vk::PhysicalDeviceBufferDeviceAddressFeaturesEXT deviceAddressfeature(true);

	createInfo.pNext = (void*)&descriptorBufferfeature;
	descriptorBufferfeature.pNext = (void*)&deviceAddressfeature;
}

void DescriptorBufferExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &descriptorProperties;

	gpu.getProperties2(&props, *Vulkan::dispatcher);

	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Descriptor Shader!")
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build(device);

	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Uniform Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithDescriptorBufferAccess() //New!
		.Build(device);

	device.getDescriptorSetLayoutSizeEXT(*descriptorLayout, &descriptorBufferSize, *Vulkan::dispatcher);
	//Now we can construct a buffer for our descriptor set!
	descriptorBuffer = CreateBuffer(descriptorBufferSize, 
		vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT,
		vk::MemoryPropertyFlagBits::eHostVisible);

	Vulkan::SetDebugName(device, vk::ObjectType::eBuffer, Vulkan::GetVulkanHandle(*descriptorBuffer.buffer), "Descriptor Buffer");

	uniformData[0] = CreateBuffer(sizeof(positionUniform)	, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	uniformData[1] = CreateBuffer(sizeof(colourUniform)		, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	uniformDataAddresses[0] = device.getBufferAddress(vk::BufferDeviceAddressInfo(*uniformData[0].buffer), *Vulkan::dispatcher);
	uniformDataAddresses[1] = device.getBufferAddress(vk::BufferDeviceAddressInfo(*uniformData[1].buffer), *Vulkan::dispatcher);

	vk::DescriptorAddressInfoEXT testAddresses[2] = {
		vk::DescriptorAddressInfoEXT(uniformDataAddresses[0], uniformData[0].requestedSize), //make this 12 to break everything!
		vk::DescriptorAddressInfoEXT(uniformDataAddresses[1], uniformData[1].requestedSize)
	};

	vk::DescriptorGetInfoEXT getInfos[2] = {
		vk::DescriptorGetInfoEXT(vk::DescriptorType::eUniformBuffer, &testAddresses[0]),
		vk::DescriptorGetInfoEXT(vk::DescriptorType::eUniformBuffer, &testAddresses[1])
	};	
	
	vk::DeviceSize		offsets[2] = {
		device.getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 0, *Vulkan::dispatcher),
		device.getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 1, *Vulkan::dispatcher)
	};
	//This DEFINITELY gets written to all 16 bytes, with different data
	void* descriptorBufferMemory  = device.mapMemory(*descriptorBuffer.deviceMem, 0, descriptorBuffer.allocInfo.allocationSize);
	device.getDescriptorEXT(&getInfos[0], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[0], *Vulkan::dispatcher);
	device.getDescriptorEXT(&getInfos[1], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[1], *Vulkan::dispatcher);
	device.unmapMemory(descriptorBuffer.deviceMem.get());

	descriptorBufferAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo(*descriptorBuffer.buffer), *Vulkan::dispatcher);

	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithDescriptorBuffers() //New!
	.Build(device);
}

void DescriptorBufferExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
		.BeginRendering(defaultCmdBuffer);

	UploadBufferData(uniformData[0], (void*)&positionUniform, sizeof(positionUniform));
	UploadBufferData(uniformData[1], (void*)&colourUniform, sizeof(colourUniform));

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);

	vk::DescriptorBufferBindingInfoEXT bindingInfo;
	bindingInfo.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
	bindingInfo.address		= descriptorBufferAddress;

	defaultCmdBuffer.bindDescriptorBuffersEXT(1, &bindingInfo, *Vulkan::dispatcher);

	uint32_t indices[1] = {
		0 //Which buffers are each set being taken from?
	};
	VkDeviceSize offsets[1] = {
		0 //How many bytes into the bound buffer is each descriptor set?
	};

	defaultCmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,	//First descriptor set
		1,	//Number of descriptor sets 
		indices,
		offsets,
		*Vulkan::dispatcher);

	SubmitDrawCall(*triMesh, defaultCmdBuffer);

 	EndRendering(defaultCmdBuffer);
}