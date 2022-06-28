/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicDescriptorRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicDescriptorRenderer::BasicDescriptorRenderer(Window& window) : VulkanTutorialRenderer(window)	{	
	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Descriptor Shader!")
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
	.Build(device);

	BuildPipeline();
}

void BasicDescriptorRenderer::BuildPipeline() {
	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Uniform Data")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device);

	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDescriptorSetLayout(*descriptorLayout)
	.Build(device, pipelineCache);

	descriptorSet = BuildUniqueDescriptorSet(*descriptorLayout);

	uniformData[0] = CreateBuffer(sizeof(positionUniform), vk::BufferUsageFlagBits::eUniformBuffer,vk::MemoryPropertyFlagBits::eHostVisible);
	uniformData[1] = CreateBuffer(sizeof(colourUniform), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	UpdateBufferDescriptor(*descriptorSet, uniformData[0], 0, vk::DescriptorType::eUniformBuffer);
	UpdateBufferDescriptor(*descriptorSet, uniformData[1], 1, vk::DescriptorType::eUniformBuffer);
}

void BasicDescriptorRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	UploadBufferData(uniformData[0], (void*)&positionUniform, sizeof(positionUniform));
	UploadBufferData(uniformData[1], (void*)&colourUniform  , sizeof(colourUniform));

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	SubmitDrawCall(*triMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}