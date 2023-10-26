/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "OffsetUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int NUM_OBJECTS = 4;

OffsetUniformBufferRenderer::OffsetUniformBufferRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

OffsetUniformBufferRenderer::~OffsetUniformBufferRenderer() {
}

void OffsetUniformBufferRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	triMesh = GenerateTriangle();

	objectMatrixLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithDynamicUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build("Object Matrices");

	size_t uboSize = GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	objectMatrixData = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(uboSize * NUM_OBJECTS, "Object Matrices");

	char* objectMats = (char*)objectMatrixData.Map();

	for (int i = 0; i < NUM_OBJECTS; ++i) {
		Matrix4* matrix = (Matrix4*)(&objectMats[uboSize * i]);
		*matrix = Matrix4::Translation({i * 5.0f, 0, 0});
	}

	objectMatrixData.Unmap();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("OffsetUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build("Offset Uniform Buffer Usage!");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)
		.WithDescriptorSetLayout(1, *objectMatrixLayout)
		.Build("UBO Pipeline");

	objectMatrixSet = BuildUniqueDescriptorSet(*objectMatrixLayout);

	WriteBufferDescriptor(*objectMatrixSet, 0, vk::DescriptorType::eUniformBufferDynamic,  objectMatrixData , 0, sizeof(Matrix4));
}

void OffsetUniformBufferRenderer::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);
}

void OffsetUniformBufferRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	size_t uboSize = GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	for (int i = 0; i < NUM_OBJECTS; ++i) {
		uint32_t uboOffset = uboSize * i;

		frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*objectMatrixSet, 1, &uboOffset);
		DrawMesh(frameCmds, *triMesh);
	}
}