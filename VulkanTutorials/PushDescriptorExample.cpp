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

PushDescriptorExample::PushDescriptorExample(Window& window) : VulkanTutorialRenderer(window) {
}

void PushDescriptorExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
}

void PushDescriptorExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	vk::PhysicalDeviceProperties2 physicalProps;
	vk::PhysicalDevicePushDescriptorPropertiesKHR pushProps;
	physicalProps.pNext = &pushProps;
	GetPhysicalDevice().getProperties2(&physicalProps);
	std::cout << __FUNCTION__ << ": Physical device supports " << pushProps.maxPushDescriptors << " push descriptors\n";

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build("Basic Descriptor Shader!");

	descriptorLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR)	//New!
		.Build("Uniform Data");

	uniformData[0] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Position Uniform");

	uniformData[1] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(colourUniform), "Colour Uniform");

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

void PushDescriptorExample::RenderFrame() {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorBufferInfo bufferInfo[2] = {
		{ uniformData[0].buffer, 0, VK_WHOLE_SIZE },
		{ uniformData[1].buffer, 0, VK_WHOLE_SIZE }
	};

	vk::WriteDescriptorSet pushSets[2];

	pushSets[0].setDstBinding(0)
			   .setDescriptorCount(1)
			   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
			   .setBufferInfo(bufferInfo[0]);

	pushSets[1].setDstBinding(1)
			   .setDescriptorCount(1)
			   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
			   .setBufferInfo(bufferInfo[1]);

	frameCmds.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 2, pushSets);

	DrawMesh(frameCmds, *triMesh);
}