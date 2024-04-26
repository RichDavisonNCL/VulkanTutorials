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

AsyncComputeExample::AsyncComputeExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	vkInit.autoBeginDynamicRendering = false;
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	frameID = 0;

	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	particlePositions = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(Vector4) * PARTICLE_COUNT, "Particles!");

	frameIDBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(uint32_t), "Frame ID");

	dataLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Data"); //Get our camera matrices...

	bufferDescriptor = CreateDescriptorSet(device, pool, *dataLayout);
	WriteBufferDescriptor(device,*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
	WriteBufferDescriptor(device,*bufferDescriptor, 1, vk::DescriptorType::eStorageBuffer, frameIDBuffer);

	BuildComputePipeline();
	BuildRasterPipeline();

	vk::CommandPool asyncPool = renderer->GetCommandPool(CommandBuffer::AsyncCompute);
	asyncBuffer = CmdBufferBegin(device, asyncPool, "Async cmds");
}

void	AsyncComputeExample::BuildRasterPipeline() {
	rasterShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("AsyncCompute.frag.spv")
		.Build("Shader using compute data!");

	basicPipeline = PipelineBuilder(renderer->GetDevice())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.Build("Raster Pipeline");
}

void	AsyncComputeExample::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(renderer->GetDevice(), "AsyncCompute.comp.spv"));

	computePipeline = ComputePipelineBuilder(renderer->GetDevice())
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.Build("Compute Pipeline");
}

void AsyncComputeExample::RenderFrame(float dt) {
	//Compute goes outside of a render pass...

	FrameState const& state = renderer->GetFrameState();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	vk::Queue		asyncQueue = renderer->GetQueue(CommandBuffer::AsyncCompute);

	vk::CommandBuffer cmdBuffer = state.cmdBuffer;

	asyncQueue.waitIdle();

	asyncBuffer->reset();
	asyncBuffer->begin(vk::CommandBufferBeginInfo());
	asyncBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	asyncBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	asyncBuffer->pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);
	asyncBuffer->pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, sizeof(float), sizeof(uint32_t), (void*)&frameID);

	asyncBuffer->dispatch(PARTICLE_COUNT / 32, 1, 1);
	CmdBufferEndSubmit(*asyncBuffer, asyncQueue);

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eVertexShader,
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithColourAttachment(state.colourView)
			.WithRenderArea(state.defaultScreenRect)
			.Build()
	);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	cmdBuffer.pushConstants(*basicPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&frameID);
	cmdBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

	cmdBuffer.endRendering();
	frameID++;
}
