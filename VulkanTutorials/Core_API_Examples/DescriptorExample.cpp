/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "DescriptorExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(DescriptorExample)

DescriptorExample::DescriptorExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
	.Build("Basic Descriptor Shader!");
	
	descriptorLayout = DescriptorSetLayoutBuilder(device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.WithUniformBuffers(1, 1, vk::ShaderStageFlagBits::eFragment)
	.Build("Uniform Data");

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithDescriptorSetLayout(0, *descriptorLayout)
	.Build("Basic Descriptor pipeline");

	descriptorSet = CreateDescriptorSet(device, pool, *descriptorLayout);

	uniformData[0] = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Positions");

	uniformData[1] = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Colours");

	WriteDescriptor(device,
		{
			.dstSet = *descriptorSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer
		},
		{
			uniformData[0]
		}
	);

	//WriteBufferDescriptor(device, *descriptorSet, 0, vk::DescriptorType::eUniformBuffer, uniformData[0]);
	WriteBufferDescriptor(device, *descriptorSet, 1, vk::DescriptorType::eUniformBuffer, uniformData[1]);
}

void DescriptorExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	triMesh->Draw(state.cmdBuffer);
}