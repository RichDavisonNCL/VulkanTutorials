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

DescriptorBufferExample::DescriptorBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
//	vkInit.autoBeginDynamicRendering = false;
// 
	vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);	
	
	vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	static vk::PhysicalDeviceDescriptorBufferFeaturesEXT	descriptorBufferfeature = {
		.descriptorBuffer = true
	};
	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
	};

	vkInit.features.push_back((void*)&descriptorBufferfeature);
	vkInit.features.push_back((void*)&deviceAddressfeature);


	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &descriptorProperties;

	renderer->GetPhysicalDevice().getProperties2(&props);

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build("Basic Descriptor Shader!");

	descriptorLayout = DescriptorSetLayoutBuilder(renderer->GetDevice())
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithUniformBuffers(1, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT)	//New!
		.Build("Uniform Data");

	renderer->GetDevice().getDescriptorSetLayoutSizeEXT(*descriptorLayout, &descriptorBufferSize);

	descriptorBuffer = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
							vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT | 
							vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT)
		.WithHostVisibility()
		.Build(descriptorBufferSize, "Descriptor Buffer");

	uniformData[0] = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Position Uniform");

	uniformData[1] = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(colourUniform), "Colour Uniform");

	DescriptorBufferWriter(renderer->GetDevice(), *descriptorLayout, descriptorBuffer)
		.SetProperties(&descriptorProperties)
		.WriteBuffer(0, vk::DescriptorType::eUniformBuffer, uniformData[0])
		.WriteBuffer(1, vk::DescriptorType::eUniformBuffer, uniformData[1])
		.Finish();

	FrameState const& frameState = renderer->GetFrameState();
	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithDescriptorBuffers() //New!
	.Build("Descriptor Buffer Pipeline");
}

void DescriptorBufferExample::RenderFrame(float dt) {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	FrameState const& frameState = renderer->GetFrameState();

	frameState.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferBindingInfoEXT bindingInfo;
	bindingInfo.usage		= vk::BufferUsageFlagBits::eShaderDeviceAddress 
							| vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT 
							| vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
	bindingInfo.address		= descriptorBuffer.deviceAddress;

	frameState.cmdBuffer.bindDescriptorBuffersEXT(1, &bindingInfo);

	uint32_t indices[1] = {
		0 //Which buffers from the above binding are each set being taken from?
	};
	VkDeviceSize offsets[1] = {
		0 //How many bytes into the bound buffer is each descriptor set?
	};

	frameState.cmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,	//First descriptor set
		1,	//Number of descriptor sets 
		indices,
		offsets);

	triMesh->Draw(frameState.cmdBuffer);
}