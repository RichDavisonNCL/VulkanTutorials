/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "UniformBufferExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

UniformBufferExample::UniformBufferExample(Window& window) : VulkanTutorial(window)	{
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
		.Build("Basic Uniform Buffer Usage!");

	cameraData = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * 2, "Camera Data");

	cameraMemory = (Matrix4*)cameraData.Map();
	camera.SetPosition({ 0,0,10 });

	FrameState const& frameState = renderer->GetFrameState();

	pipeline = PipelineBuilder(renderer->GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.Build("UBO Pipeline");

	descriptorSet = CreateDescriptorSet(renderer->GetDevice(), renderer->GetDescriptorPool(), shader->GetLayout(0));

	WriteBufferDescriptor(renderer->GetDevice() , *descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraData);
}

UniformBufferExample::~UniformBufferExample() {
	cameraData.Unmap();
}

void UniformBufferExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	camera.UpdateCamera(dt);

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform();

	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	triMesh->Draw(state.cmdBuffer);
}

void UniformBufferExample::UpdateCameraUniform() {
	Matrix4 camMatrices[2];
	camMatrices[0] = camera.BuildViewMatrix();
	camMatrices[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());
	//'Traditional' method...
	//UpdateUniform(cameraData, (void*)&camMatrices, sizeof(Matrix4) * 2);

	//'Modern' method - just permamap the buffer and transfer the new data directly to GPU
	//Only works with the host visible bit set...
	cameraMemory[0] = camMatrices[0];
	cameraMemory[1] = camMatrices[1];
}