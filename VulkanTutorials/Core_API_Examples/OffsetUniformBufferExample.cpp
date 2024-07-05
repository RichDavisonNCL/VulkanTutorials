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

//TUTORIAL_ENTRY(OffsetUniformBufferExample)

//OffsetUniformBufferExample::OffsetUniformBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorialRenderer(window)	{
//}
//
//OffsetUniformBufferExample::~OffsetUniformBufferExample() {
//}
//
//void OffsetUniformBufferExample::SetupTutorial() {
//	VulkanTutorial::SetupTutorial();
//
//	triMesh = GenerateTriangle();
//
//	vk::Device device = renderer->GetDevice();
//
//	shader = ShaderBuilder(device)
//		.WithVertexBinary("OffsetUniformBuffer.vert.spv")
//		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
//	.Build("Offset Uniform Buffer Usage!");
//
//	objectMatrixLayout = DescriptorSetLayoutBuilder(device)
//		.WithDynamicUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
//	.Build("Object Matrices");
//
//	size_t uboSize = renderer->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
//
//	objectMatrixData = BufferBuilder(device, renderer->GetMemoryAllocator())
//		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
//		.WithHostVisibility()
//		.Build(uboSize * NUM_OBJECTS, "Object Matrices");
//
//	char* objectMats = (char*)objectMatrixData.Map();
//
//	for (int i = 0; i < NUM_OBJECTS; ++i) {
//		Matrix4* matrix = (Matrix4*)(&objectMats[uboSize * i]);
//		*matrix = Matrix::Translation(Vector3{i * 5.0f, 0, 0});
//	}
//
//	objectMatrixData.Unmap();
//
//	pipeline = PipelineBuilder(device)
//		.WithVertexInputState(triMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleList)
//		.WithShader(shader)
//		.WithColourAttachment(renderer->GetSurfaceFormat())
//		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat())
//		.WithDescriptorSetLayout(0, cameraLayout)
//		.WithDescriptorSetLayout(1, objectMatrixLayout)
//		.Build("UBO Pipeline");
//
//	objectMatrixSet = CreateDescriptorSet(device, *objectMatrixLayout);
//
//	WriteBufferDescriptor(device, *objectMatrixSet, 0, vk::DescriptorType::eUniformBufferDynamic,  objectMatrixData , 0, sizeof(Matrix4));
//}
//
//void OffsetUniformBufferExample::Update(float dt) {
//	VulkanTutorial::Update(dt);
//}
//
//void OffsetUniformBufferExample::RenderFrame() {
//	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
//
//	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
//
//	size_t uboSize = renderer->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
//
//	for (int i = 0; i < NUM_OBJECTS; ++i) {
//		uint32_t uboOffset = uboSize * i;
//
//		frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*objectMatrixSet, 1, &uboOffset);
//		triMesh->Draw(frameCmds);
//	}
//}