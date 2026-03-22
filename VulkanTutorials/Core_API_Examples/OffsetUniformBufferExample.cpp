/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "OffsetUniformBufferExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int NUM_OBJECTS = 4;

TUTORIAL_ENTRY(OffsetUniformBufferExample)

OffsetUniformBufferExample::OffsetUniformBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	objectMatrixLayout = DescriptorSetLayoutBuilder(context.device)
		.WithDynamicUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
	.Build("Object Matrices");

	size_t uboSize = m_renderer->GetPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;

	objectMatrixData = m_memoryManager->CreateBuffer(
		{
			.size = uboSize * NUM_OBJECTS,
			.usage = vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Object Matrices"
	);

	char* objectMats = (char*)objectMatrixData.Map();

	for (int i = 0; i < NUM_OBJECTS; ++i) {
		Matrix4* matrix = (Matrix4*)(&objectMats[uboSize * i]);
		*matrix = Matrix::Translation(Vector3{i * 5.0f, 0, 0});
	}

	objectMatrixData.Unmap();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("OffsetUniformBuffer.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicUniformBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithDescriptorSetLayout(0, m_cameraLayout)
		.WithDescriptorSetLayout(1, objectMatrixLayout)
		.Build("UBO Pipeline");

	objectMatrixSet = CreateDescriptorSet(context.device, context.descriptorPool, *objectMatrixLayout);

	WriteBufferDescriptor(context.device, *objectMatrixSet, 0, vk::DescriptorType::eUniformBufferDynamic, objectMatrixData , 0, sizeof(Matrix4));
}

void OffsetUniformBufferExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);

	size_t uboSize = m_renderer->GetPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;

	for (int i = 0; i < NUM_OBJECTS; ++i) {
		uint32_t uboOffset = uboSize * i;

		context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*objectMatrixSet, 1, &uboOffset);
		m_triangleMesh->Draw(context.cmdBuffer);
	}
}