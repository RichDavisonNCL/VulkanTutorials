/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

BasicUniformBufferRenderer::BasicUniformBufferRenderer(Window& window) : VulkanTutorialRenderer(window)	{

}

BasicUniformBufferRenderer::~BasicUniformBufferRenderer() {
	cameraData.Unmap();
}

void BasicUniformBufferRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build("Basic Uniform Buffer Usage!");

	cameraData = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * 2, "Camera Data");

	cameraMemory = (Matrix4*)cameraData.Map();
	camera.SetPosition({0,0,10});
	
	matrixLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build("Matrices");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *matrixLayout)
		.Build("UBO Pipeline");

	descriptorSet = BuildUniqueDescriptorSet(*matrixLayout);

	WriteBufferDescriptor(*descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraData);
}

void BasicUniformBufferRenderer::Update(float dt) {
	camera.UpdateCamera(dt);
}

void BasicUniformBufferRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform();

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	DrawMesh(frameCmds, *triMesh);
}

void BasicUniformBufferRenderer::UpdateCameraUniform() {
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