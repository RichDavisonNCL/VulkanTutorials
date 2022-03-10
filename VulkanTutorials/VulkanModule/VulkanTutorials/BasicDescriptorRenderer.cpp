/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicDescriptorRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicDescriptorRenderer::BasicDescriptorRenderer(Window& window) : VulkanTutorialRenderer(window)	{	
	triMesh = std::shared_ptr<VulkanMesh>((VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh()));
	triMesh->UploadToGPU(this);

	shader = VulkanShaderBuilder()
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
		.WithDebugName("Basic Descriptor Shader!")
	.BuildUnique(device);

	BuildPipeline();
}

BasicDescriptorRenderer::~BasicDescriptorRenderer()	{
}

void BasicDescriptorRenderer::BuildPipeline() {
	descriptorLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("UniformData")
	.BuildUnique(device); //Get our camera matrices...

	pipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader.get())
		.WithPass(defaultRenderPass)
		.WithDescriptorSetLayout(descriptorLayout.get())
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithRaster(vk::CullModeFlagBits::eNone)
	.Build(device, pipelineCache);

	descriptorSet = BuildUniqueDescriptorSet(descriptorLayout.get());

	uniformData[0] = CreateBuffer(sizeof(positionUniform), vk::BufferUsageFlagBits::eUniformBuffer,vk::MemoryPropertyFlagBits::eHostVisible);
	uniformData[1] = CreateBuffer(sizeof(colourUniform), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	UpdateUniformBufferDescriptor(descriptorSet.get(), uniformData[0], 0);
	UpdateUniformBufferDescriptor(descriptorSet.get(), uniformData[1], 1);
}

void BasicDescriptorRenderer::RenderFrame() {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	BeginDefaultRenderPass(defaultCmdBuffer);

	UploadBufferData(uniformData[0], (void*)&positionUniform, sizeof(positionUniform));
	UploadBufferData(uniformData[1], (void*)&colourUniform  , sizeof(colourUniform));

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 0, 1, &descriptorSet.get(), 0, nullptr);

	SubmitDrawCall(triMesh.get(), defaultCmdBuffer);

	defaultCmdBuffer.endRenderPass();
}