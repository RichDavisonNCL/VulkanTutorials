/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicUniformBufferRenderer::BasicUniformBufferRenderer(Window& window) : VulkanRenderer(window)	{
	camera				= Camera::BuildPerspectiveCamera(Vector3(0, 0, 1), 0, 0, 45.0f, 0.1f, 100.0f);

	triMesh = std::shared_ptr<VulkanMesh>((VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh()));
	triMesh->UploadToGPU(this);

	defaultShader = VulkanShaderBuilder()
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
		.WithDebugName("Basic Uniform Buffer Usage!")
	.BuildUnique(device);

	cameraData		= CreateBuffer(sizeof(Matrix4) * 2, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	cameraMemory	= (Matrix4*)device.mapMemory(cameraData.deviceMem.get(), 0, cameraData.allocInfo.allocationSize);
	
	BuildPipeline();
}

BasicUniformBufferRenderer::~BasicUniformBufferRenderer()	{	
}

void BasicUniformBufferRenderer::Update(float msec) {
	camera.UpdateCamera(msec);
}

void	BasicUniformBufferRenderer::BuildPipeline() {
	vk::DescriptorSetLayout basicLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithDebugName("Matrices")
	.Build(device);

	basicPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(defaultShader.get())
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(basicLayout)
		.WithDebugName("UBOPipeline")
	.Build(device, pipelineCache);

	descriptorSet = BuildUniqueDescriptorSet(basicLayout);
	WriteDescriptorSet(descriptorSet.get());

	device.destroyDescriptorSetLayout(basicLayout);
}

void BasicUniformBufferRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder rendering = VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view, vk::ImageLayout::eColorAttachmentOptimal, true, ClearColour(0.2f, 0.2f, 0.2f, 1.0f))
		.WithDepthAttachment(depthBuffer->GetDefaultView(), vk::ImageLayout::eDepthAttachmentOptimal, true, { 1.0f }, false)
		.WithRenderArea(defaultScreenRect);

	rendering.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline.pipeline.get());

	UpdateCameraUniform();

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, basicPipeline.layout.get(), 0, 1, &descriptorSet.get(), 0, nullptr);

	SubmitDrawCall(triMesh.get(), defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}

void BasicUniformBufferRenderer::WriteDescriptorSet(vk::DescriptorSet set)	{
	vk::WriteDescriptorSet descriptorWrites[1] = {
		vk::WriteDescriptorSet()
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstSet(set)
			.setDstBinding(0)
			.setDescriptorCount(1)
			.setPBufferInfo(&cameraData.descriptorInfo)
	};
	device.updateDescriptorSets(1, descriptorWrites, 0, nullptr);
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