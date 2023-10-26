/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "AsyncComputeExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int PARTICLE_COUNT = 32 * 1000;

AsyncComputeExample::AsyncComputeExample(Window& window) : VulkanTutorialRenderer(window) {
	autoBeginDynamicRendering = false;
	frameID = 0;
}

void AsyncComputeExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	particlePositions = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(Vector4) * PARTICLE_COUNT, "Particles!");

	frameIDBuffer = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(uint32_t), "Frame ID");

	dataLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Data"); //Get our camera matrices...

	bufferDescriptor = BuildUniqueDescriptorSet(*dataLayout);
	WriteBufferDescriptor(*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
	WriteBufferDescriptor(*bufferDescriptor, 1, vk::DescriptorType::eStorageBuffer, frameIDBuffer);

	BuildComputePipeline();
	BuildRasterPipeline();

	vk::CommandPool asyncPool = GetCommandPool(CommandBuffer::AsyncCompute);
	asyncBuffer = CmdBufferBegin(GetDevice(), asyncPool, "Async cmds");
}

void	AsyncComputeExample::BuildRasterPipeline() {
	rasterShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("AsyncCompute.frag.spv")
		.Build("Shader using compute data!");

	basicPipeline = PipelineBuilder(GetDevice())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.WithPushConstant(vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t))
		.Build("Raster Pipeline");
}

void	AsyncComputeExample::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "AsyncCompute.comp.spv"));

	computePipeline = ComputePipelineBuilder(GetDevice())
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float) + sizeof(uint32_t))
		.Build("Compute Pipeline");
}

void AsyncComputeExample::RenderFrame() {
	//Compute goes outside of a render pass...
	vk::Queue		asyncQueue = GetQueue(CommandBuffer::AsyncCompute);

	asyncQueue.waitIdle();

	asyncBuffer->reset();
	asyncBuffer->begin(vk::CommandBufferBeginInfo());
	asyncBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	asyncBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	asyncBuffer->pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);
	asyncBuffer->pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, sizeof(float), sizeof(uint32_t), (void*)&frameID);

	asyncBuffer->dispatch(PARTICLE_COUNT / 32, 1, 1);
	CmdBufferEndSubmit(*asyncBuffer, asyncQueue);

	frameCmds.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eVertexShader,
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	DynamicRenderBuilder()
		.WithColourAttachment(GetCurrentSwapView())
		.WithRenderArea(defaultScreenRect)
		.BeginRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	frameCmds.pushConstants(*basicPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&frameID);
	frameCmds.draw(PARTICLE_COUNT, 1, 0, 0);

	frameCmds.endRendering();
	frameID++;
}