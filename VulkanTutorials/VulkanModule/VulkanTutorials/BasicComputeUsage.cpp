/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicComputeUsage.h"

using namespace NCL;
using namespace Rendering;

const int PARTICLE_COUNT = 32 * 10;

BasicComputeUsage::BasicComputeUsage(Window& window) : VulkanTutorialRenderer(window)	{
	particleMesh = UniqueVulkanMesh(new VulkanMesh());
	particleMesh->SetPrimitiveType(NCL::GeometryPrimitive::Points);
	particleMesh->SetVertexPositions(std::vector<Vector3>(PARTICLE_COUNT));
	particleMesh->UploadToGPU(this);

	particlePositions = CreateBuffer(sizeof(Vector4) * PARTICLE_COUNT, 
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer, 
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	BuildComputePipeline();
	BuildRasterPipeline();
}

void	BasicComputeUsage::BuildRasterPipeline() {
	rasterShader = VulkanShaderBuilder("Shader using compute data!")
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("BasicCompute.frag.spv")
	.BuildUnique(device);

	vk::DescriptorSetLayout dataLayout = VulkanDescriptorSetLayoutBuilder("Compute Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
	.Build(device); //Get our camera matrices...

	basicPipeline = VulkanPipelineBuilder("Raster Pipeline")
		.WithVertexInputState(particleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(dataLayout)
		.WithShader(rasterShader)
	.Build(device, pipelineCache);

	device.destroyDescriptorSetLayout(dataLayout);
}

void	BasicComputeUsage::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(device, "BasicCompute.comp.spv"));

	vk::DescriptorSetLayout dataLayout = VulkanDescriptorSetLayoutBuilder("Compute Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
	.Build(device); //Get our camera matrices...

	computePipeline = VulkanComputePipelineBuilder("Compute Pipeline")
		.WithComputeState(&*computeShader)
		.WithDescriptorSetLayout(dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float))
	.Build(device, pipelineCache);

	bufferDescriptor = BuildDescriptorSet(dataLayout);

	UpdateBufferDescriptor(bufferDescriptor, particlePositions, 0, vk::DescriptorType::eStorageBuffer);

	device.destroyDescriptorSetLayout(dataLayout);
}

void BasicComputeUsage::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	//Compute goes outside of a render pass...
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &bufferDescriptor, 0, nullptr);
	defaultCmdBuffer.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);

	DispatchCompute(defaultCmdBuffer, PARTICLE_COUNT / 32, 1, 1);

	defaultCmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, 
		vk::PipelineStageFlagBits::eVertexShader, 
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
		.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *basicPipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &bufferDescriptor, 0, nullptr);
	SubmitDrawCall(*particleMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}