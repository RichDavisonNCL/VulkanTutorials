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

PushDescriptorExample::PushDescriptorExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();

	vkInit.deviceExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::PhysicalDeviceProperties2 physicalProps;
	vk::PhysicalDevicePushDescriptorPropertiesKHR pushProps;
	physicalProps.pNext = &pushProps;
	renderer->GetPhysicalDevice().getProperties2(&physicalProps);
	std::cout << __FUNCTION__ << ": Physical device supports " << pushProps.maxPushDescriptors << " push descriptors\n";

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.Build("Basic Descriptor Shader!");

	descriptorLayout = DescriptorSetLayoutBuilder(renderer->GetDevice())
		.WithDescriptors(shader->GetLayoutBinding(0))
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR)	//New!
		.Build("Uniform Data");

	uniformData[0] = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Position Uniform");

	uniformData[1] = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(colourUniform), "Colour Uniform");

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
	.Build("Descriptor Buffer Pipeline");
}

void PushDescriptorExample::RenderFrame(float dt) {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	FrameState const& state = renderer->GetFrameState();

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

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

	state.cmdBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 2, pushSets);

	triMesh->Draw(state.cmdBuffer);
}