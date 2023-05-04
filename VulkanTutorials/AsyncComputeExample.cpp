/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "AsyncComputeExample.h"

using namespace NCL;
using namespace Rendering;

const int PARTICLE_COUNT = 320 * 10000;

AsyncComputeExample::AsyncComputeExample(Window& window) : VulkanTutorialRenderer(window) {
	autoBeginDynamicRendering = false;
	frameID = 0;
}

void AsyncComputeExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	particlePositions = VulkanBufferBuilder(sizeof(Vector4) * PARTICLE_COUNT, "Particles!")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(GetDevice(), GetMemoryAllocator());

	frameIDBuffer = VulkanBufferBuilder(sizeof(uint32_t), "Frame ID")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(GetDevice(), GetMemoryAllocator());

	dataLayout = VulkanDescriptorSetLayoutBuilder("Compute Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
		.Build(GetDevice()); //Get our camera matrices...

	bufferDescriptor = BuildUniqueDescriptorSet(*dataLayout);
	UpdateBufferDescriptor(*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
	UpdateBufferDescriptor(*bufferDescriptor, 1, vk::DescriptorType::eStorageBuffer, frameIDBuffer);

	BuildComputePipeline();
	BuildRasterPipeline();
}

void	AsyncComputeExample::BuildRasterPipeline() {
	rasterShader = VulkanShaderBuilder("Shader using compute data!")
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("AsyncCompute.frag.spv")
		.Build(GetDevice());

	basicPipeline = VulkanPipelineBuilder("Raster Pipeline")
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithPushConstant(vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t))
		.Build(GetDevice());
}

void	AsyncComputeExample::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "AsyncCompute.comp.spv"));

	computePipeline = VulkanComputePipelineBuilder("Compute Pipeline")
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float) + sizeof(uint32_t))
		.Build(GetDevice());
}

void AsyncComputeExample::RenderFrame() {
	//Compute goes outside of a render pass...

	vk::CommandBuffer asyncBuffer = BeginCmdBuffer(CommandBufferType::AsyncCompute);

	asyncBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	asyncBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	asyncBuffer.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);
	asyncBuffer.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, sizeof(float), sizeof(uint32_t), (void*)&frameID);

	asyncBuffer.dispatch(PARTICLE_COUNT / 32, 1, 1);

	SubmitCmdBuffer(asyncBuffer, CommandBufferType::AsyncCompute);

	//frameCmds.pipelineBarrier(
	//	vk::PipelineStageFlagBits::eComputeShader,
	//	vk::PipelineStageFlagBits::eVertexShader,
	//	vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	//);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(GetCurrentSwapView())
		.WithRenderArea(defaultScreenRect)
		.BeginRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	frameCmds.pushConstants(*basicPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&frameID);
	frameCmds.draw(PARTICLE_COUNT, 1, 0, 0);

	EndRendering(frameCmds);
	frameID++;
}