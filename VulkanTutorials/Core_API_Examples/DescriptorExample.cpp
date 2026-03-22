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

DescriptorExample::DescriptorExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	descriptorLayout = DescriptorSetLayoutBuilder(context.device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.WithUniformBuffers(1, 1, vk::ShaderStageFlagBits::eFragment)
	.Build("Uniform Data");

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithDescriptorSetLayout(0, *descriptorLayout)
		.WithShaderBinary("BasicDescriptor.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicDescriptor.frag.spv", vk::ShaderStageFlagBits::eFragment)

	.Build("Basic Descriptor pipeline");

	descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, *descriptorLayout);

	uniformData[0] = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(positionUniform),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Positions"
	);

	uniformData[1] = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(positionUniform),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Colours"
	);

	WriteDescriptor(context.device,
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
	WriteBufferDescriptor(context.device, *descriptorSet, 1, vk::DescriptorType::eUniformBuffer, uniformData[1]);
}

void DescriptorExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	Vector3 positionUniform = Vector3(sin(m_runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(m_runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	m_triangleMesh->Draw(context.cmdBuffer);
}