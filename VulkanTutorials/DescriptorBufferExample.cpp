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

	GetPhysicalDevice().getProperties2(&props);

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build("Basic Descriptor Shader!");

	descriptorLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)	//New!
		.Build("Uniform Data");

	GetDevice().getDescriptorSetLayoutSizeEXT(*descriptorLayout, &descriptorBufferSize);

	descriptorBuffer = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
							vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | 
							vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT)
		.WithHostVisibility()
		.Build(descriptorBufferSize, "Descriptor Buffer");

	uniformData[0] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Position Uniform");

	uniformData[1] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(colourUniform), "Colour Uniform");

	vk::DescriptorAddressInfoEXT testAddresses[2] = {
		{uniformData[0].deviceAddress, uniformData[0].size}, //make this 12 to break everything!
		{uniformData[1].deviceAddress, uniformData[1].size}
	};

	vk::DescriptorGetInfoEXT getInfos[2] = {//Param 2 is a DescriptorDataEXT
		{vk::DescriptorType::eUniformBuffer, &testAddresses[0]},
		{vk::DescriptorType::eUniformBuffer, &testAddresses[1]}
	};	
	
	vk::DeviceSize		offsets[2] = {
		GetDevice().getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 0),
		GetDevice().getDescriptorSetLayoutBindingOffsetEXT(*descriptorLayout, 1)
	};

	void* descriptorBufferMemory = descriptorBuffer.Map();
	GetDevice().getDescriptorEXT(&getInfos[0], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[0]);
	GetDevice().getDescriptorEXT(&getInfos[1], descriptorProperties.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offsets[1]);
	descriptorBuffer.Unmap();

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorBuffers() //New!
	.Build("Descriptor Buffer Pipeline");
}

void DescriptorBufferExample::RenderFrame() {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferBindingInfoEXT bindingInfo;
	bindingInfo.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress 
							| vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT 
							| vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
	bindingInfo.address		= descriptorBuffer.deviceAddress;

	frameCmds.bindDescriptorBuffersEXT(1, &bindingInfo);

	uint32_t indices[1] = {
		0 //Which buffers from the above binding are each set being taken from?
	};
	VkDeviceSize offsets[1] = {
		0 //How many bytes into the bound buffer is each descriptor set?
	};

	frameCmds.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,	//First descriptor set
		1,	//Number of descriptor sets 
		indices,
		offsets);

	DrawMesh(frameCmds, *triMesh);
}