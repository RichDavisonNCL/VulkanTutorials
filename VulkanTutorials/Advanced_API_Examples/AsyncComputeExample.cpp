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

TUTORIAL_ENTRY(AsyncComputeExample)

const int PARTICLE_COUNT = 32 * 1000;

AsyncComputeExample::AsyncComputeExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
	//We need to more directly manage when our render passes are active for async compute
	vkInit.autoBeginDynamicRendering = false; 
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	frameID = 0;

	vk::Device			device	= renderer->GetDevice();
	vk::DescriptorPool	pool	= renderer->GetDescriptorPool();

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

	rasterShader = ShaderBuilder(device)
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("AsyncCompute.frag.spv")
		.Build("Shader using compute data!");

	rasterPipeline = PipelineBuilder(device)
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.Build("Raster Pipeline");

	computeShader = std::make_unique<VulkanCompute>(device, "AsyncCompute.comp.spv");
	
	computePipeline = ComputePipelineBuilder(device)
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.Build("Compute Pipeline");

	vk::CommandPool asyncPool = renderer->GetCommandPool(CommandType::AsyncCompute);

	for (int i = 0; i < 2; ++i) {
		asyncBuffers[i] = CmdBufferCreate(device, asyncPool, "Async cmds");
		asyncFences[i] = device.createFenceUnique(
			{
				.flags = vk::FenceCreateFlagBits::eSignaled
			}
		);
	}
}

AsyncComputeExample::~AsyncComputeExample() {
	vk::Queue		asyncQueue = renderer->GetQueue(CommandType::AsyncCompute);
	asyncQueue.waitIdle(); //TODO change to a cycle of async queue buffers so we don't need this...
}

void AsyncComputeExample::RenderFrame(float dt) {
	//Compute goes outside of a render pass...

	FrameState const& state = renderer->GetFrameState();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	vk::Queue		asyncQueue = renderer->GetQueue(CommandType::AsyncCompute);

	vk::CommandBuffer cmdBuffer = state.cmdBuffer;



	vk::BufferMemoryBarrier2 releaseGraphicsBarrier = {
		.srcStageMask	= vk::PipelineStageFlagBits2::eTopOfPipe,
		.srcAccessMask	= vk::AccessFlagBits2::eMemoryRead,

		.dstStageMask	= vk::PipelineStageFlagBits2::eComputeShader,
		.dstAccessMask	= vk::AccessFlagBits2::eMemoryWrite,

		.srcQueueFamilyIndex = renderer->GetQueueFamily(CommandType::Graphics),
		.dstQueueFamilyIndex = renderer->GetQueueFamily(CommandType::AsyncCompute),
		.buffer = particlePositions,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vk::BufferMemoryBarrier2 releaseComputeBarrier = {
		.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
		.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,

		.dstStageMask  = vk::PipelineStageFlagBits2::eTopOfPipe,
		.dstAccessMask = vk::AccessFlagBits2::eMemoryRead,

		.srcQueueFamilyIndex = renderer->GetQueueFamily(CommandType::AsyncCompute),
		.dstQueueFamilyIndex = renderer->GetQueueFamily(CommandType::Graphics),
		.buffer = particlePositions,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vk::Result dontcare = renderer->GetDevice().waitForFences(*asyncFences[frameID % 2], true, UINT64_MAX);
	renderer->GetDevice().resetFences(*asyncFences[frameID % 2]);

	vk::CommandBuffer asyncCmds = asyncBuffers[frameID % 2].get();
	CmdBufferResetBegin(asyncCmds);
	
	if (frameID > 0) {
		//Acquire the resource from the gfx queue
		asyncCmds.pipelineBarrier2(
			{
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &releaseGraphicsBarrier
			}
		);
	}

	asyncCmds.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	asyncCmds.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	asyncCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0			, sizeof(float)   , (void*)&runTime);
	asyncCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, sizeof(float), sizeof(uint32_t), (void*)&frameID);
	asyncCmds.dispatch(PARTICLE_COUNT / 32, 1, 1);

	//Release the buffer to the gfx queue
	asyncCmds.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseComputeBarrier
		}
	);

	CmdBufferEndSubmit(asyncCmds, asyncQueue, *asyncFences[frameID % 2]);

	//Aquire the buffer from the compute queue
	cmdBuffer.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseComputeBarrier
		}
	);

	cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithColourAttachment(state.colourView)
			.WithRenderArea(state.defaultScreenRect)
			.Build()
	);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rasterPipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *rasterPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	cmdBuffer.pushConstants(*rasterPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&frameID);
	cmdBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

	cmdBuffer.endRendering();

	//Release the buffer from the gfx queue
	cmdBuffer.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseGraphicsBarrier
		}
	);

	frameID++;
}
