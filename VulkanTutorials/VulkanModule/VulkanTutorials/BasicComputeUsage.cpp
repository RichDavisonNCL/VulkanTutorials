/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicComputeUsage.h"

using namespace NCL;
using namespace Rendering;

const int PARTICLE_COUNT = 32 * 10;

BasicComputeUsage::BasicComputeUsage(Window& window) : VulkanTutorialRenderer(window)	{
	particleMesh = std::shared_ptr<VulkanMesh>(new VulkanMesh());
	particleMesh->SetPrimitiveType(NCL::GeometryPrimitive::Points);
	particleMesh->SetVertexPositions(std::vector<Vector3>(PARTICLE_COUNT));
	particleMesh->UploadToGPU(this);

	particlePositions = CreateBuffer(sizeof(Vector4) * PARTICLE_COUNT, 
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer, 
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	BuildComputePipeline();
	BuildRasterPipeline();
}

BasicComputeUsage::~BasicComputeUsage()	{
	//DestroyBuffer(particlePositions);
}

void	BasicComputeUsage::BuildRasterPipeline() {
	rasterShader = VulkanShaderBuilder()
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("BasicCompute.frag.spv")
		.WithDebugName("Shader using compute data!")
	.BuildUnique(device);

	vk::DescriptorSetLayout dataLayout = VulkanDescriptorSetLayoutBuilder()
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithDebugName("Compute Data")
	.Build(device); //Get our camera matrices...

	basicPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(particleMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(dataLayout)
		.WithShaderState(rasterShader.get())
		.WithPass(defaultRenderPass)
		.WithDebugName("Raster Pipeline")
	.Build(device, pipelineCache);

	device.destroyDescriptorSetLayout(dataLayout);
}

void	BasicComputeUsage::BuildComputePipeline() {
	computeShader = std::shared_ptr<VulkanCompute>(new VulkanCompute(device, "BasicCompute.comp.spv"));

	vk::DescriptorSetLayout dataLayout = VulkanDescriptorSetLayoutBuilder()
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithDebugName("Compute Data")
	.Build(device); //Get our camera matrices...

	computePipeline = VulkanComputePipelineBuilder()
		.WithComputeState(computeShader.get())
		.WithDescriptorSetLayout(dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float))
		.WithDebugName("Compute Pipeline")
	.Build(device, pipelineCache);

	bufferDescriptor = BuildDescriptorSet(dataLayout);

	vk::WriteDescriptorSet descriptorWrites = vk::WriteDescriptorSet()
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setDstSet(bufferDescriptor)
		.setDstBinding(0)
		.setDescriptorCount(1)
		.setPBufferInfo(&particlePositions.descriptorInfo);

	device.updateDescriptorSets(1, &descriptorWrites, 0, nullptr);
	device.destroyDescriptorSetLayout(dataLayout);
}

void BasicComputeUsage::RenderFrame() {
	//Compute goes outside of a render pass...
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.pipeline.get());
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipeline.layout.get(), 0, 1, &bufferDescriptor, 0, nullptr);
	defaultCmdBuffer.pushConstants(computePipeline.layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);

	DispatchCompute(defaultCmdBuffer, PARTICLE_COUNT / 32, 1, 1); //Bit a pointless function?

	defaultCmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, 
		vk::PipelineStageFlagBits::eVertexShader, 
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	BeginDefaultRenderPass(defaultCmdBuffer);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline.pipeline.get());
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, basicPipeline.layout.get(), 0, 1, &bufferDescriptor, 0, nullptr);
	SubmitDrawCall(particleMesh.get(), defaultCmdBuffer);
	defaultCmdBuffer.endRenderPass();
}