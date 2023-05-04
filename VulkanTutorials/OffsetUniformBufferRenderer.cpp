/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "OffsetUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;

const int NUM_OBJECTS = 4;

OffsetUniformBufferRenderer::OffsetUniformBufferRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

OffsetUniformBufferRenderer::~OffsetUniformBufferRenderer() {
}

void OffsetUniformBufferRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	camera	= Camera::BuildPerspectiveCamera(Vector3(0, 0, 10.0f), 0, 0, 45.0f, 1.0f, 100.0f);

	triMesh = GenerateTriangle();

	objectMatrixLayout = VulkanDescriptorSetLayoutBuilder("Object Matrices")
		.WithDynamicUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build(GetDevice());

	size_t uboSize = GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	objectMatrixData = VulkanBufferBuilder(uboSize * NUM_OBJECTS, "Object Matrices")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	char* objectMats = (char*)objectMatrixData.Map();

	for (int i = 0; i < NUM_OBJECTS; ++i) {
		Matrix4* matrix = (Matrix4*)(&objectMats[uboSize * i]);
		*matrix = Matrix4::Translation({i * 5.0f, 0, 0});
	}

	objectMatrixData.Unmap();

	shader = VulkanShaderBuilder("Offset Uniform Buffer Usage!")
		.WithVertexBinary("OffsetUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build(GetDevice());

	pipeline = VulkanPipelineBuilder("UBOPipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)
		.WithDescriptorSetLayout(1, *objectMatrixLayout)
		.Build(GetDevice());

	objectMatrixSet = BuildUniqueDescriptorSet(*objectMatrixLayout);

	UpdateBufferDescriptor(*objectMatrixSet, 0, vk::DescriptorType::eUniformBufferDynamic,  objectMatrixData , 0, sizeof(Matrix4));
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
		SubmitDrawCall(frameCmds, *triMesh);
	}
}