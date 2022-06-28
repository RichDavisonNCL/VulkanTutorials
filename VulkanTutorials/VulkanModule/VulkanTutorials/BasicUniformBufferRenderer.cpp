/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicUniformBufferRenderer::BasicUniformBufferRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	camera	= Camera::BuildPerspectiveCamera(Vector3(0, 0, 10.0f), 0, 0, 45.0f, 1.0f, 100.0f);

	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Uniform Buffer Usage!")
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build(device);

	cameraData		= CreateBuffer(sizeof(Matrix4) * 2, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	cameraMemory	= (Matrix4*)device.mapMemory(*cameraData.deviceMem, 0, cameraData.allocInfo.allocationSize);
	
	BuildPipeline();
}

void BasicUniformBufferRenderer::Update(float dt) {
	camera.UpdateCamera(dt);
}

void	BasicUniformBufferRenderer::BuildPipeline() {
	matrixLayout = VulkanDescriptorSetLayoutBuilder("Matrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build(device);

	pipeline = VulkanPipelineBuilder("UBOPipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourFormats({ surfaceFormat })
		.WithDescriptorSetLayout(*matrixLayout)
	.Build(device, pipelineCache);

	descriptorSet = BuildUniqueDescriptorSet(*matrixLayout);

	UpdateBufferDescriptor(*descriptorSet, cameraData, 0, vk::DescriptorType::eUniformBuffer);
}

void BasicUniformBufferRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder rendering = VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view, vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.2f, 0.2f, 0.2f, 1.0f))
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);

	UpdateCameraUniform();

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	SubmitDrawCall(*triMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
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