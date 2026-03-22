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

AsyncComputeExample::AsyncComputeExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	//We need to more directly manage when our render passes are active for async compute
	m_vkInit.autoBeginDynamicRendering = false; 
	Initialise();

	frameID = 0;

	FrameContext const& context = m_renderer->GetFrameContext();

	particlePositions = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(Vector4) * PARTICLE_COUNT,
			.usage = vk::BufferUsageFlagBits::eStorageBuffer
		},
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		"Particles!"
	);

	frameIDBuffer = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(uint32_t),
			.usage = vk::BufferUsageFlagBits::eStorageBuffer
		},
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		"Frame ID"
	);

	dataLayout = DescriptorSetLayoutBuilder(context.device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Data"); //Get our m_camera matrices...

	bufferDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, *dataLayout);
	WriteBufferDescriptor(context.device,*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
	WriteBufferDescriptor(context.device,*bufferDescriptor, 1, vk::DescriptorType::eStorageBuffer, frameIDBuffer);

	rasterPipeline = PipelineBuilder(context.device)
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShaderBinary("BasicCompute.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("AsyncCompute.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.Build("Raster Pipeline");

	computePipeline = ComputePipelineBuilder(context.device)
		.WithShaderBinary("BasicCompute.comp.spv")
		.WithDescriptorSetLayout(0, *dataLayout)
		.Build("Compute Pipeline");

	for (int i = 0; i < 2; ++i) {
		asyncBuffers[i] = CmdBufferCreate(context.device, context.commandPools[CommandType::AsyncCompute], "Async cmds");
		asyncFences[i] = context.device.createFenceUnique(
			{
				.flags = vk::FenceCreateFlagBits::eSignaled
			}
		);
	}
}

AsyncComputeExample::~AsyncComputeExample() {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.queues[CommandType::AsyncCompute].waitIdle(); //TODO change to a cycle of async queue buffers so we don't need this...
}

void AsyncComputeExample::RenderFrame(float dt) {
	//Compute goes outside of a render pass...
	FrameContext const& context = m_renderer->GetFrameContext();

	vk::BufferMemoryBarrier2 releaseGraphicsBarrier = {
		.srcStageMask	= vk::PipelineStageFlagBits2::eTopOfPipe,
		.srcAccessMask	= vk::AccessFlagBits2::eMemoryRead,

		.dstStageMask	= vk::PipelineStageFlagBits2::eComputeShader,
		.dstAccessMask	= vk::AccessFlagBits2::eMemoryWrite,

		.srcQueueFamilyIndex = context.queueFamilies[CommandType::Graphics],
		.dstQueueFamilyIndex = context.queueFamilies[CommandType::AsyncCompute],
		.buffer = particlePositions.buffer,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vk::BufferMemoryBarrier2 releaseComputeBarrier = {
		.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
		.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,

		.dstStageMask  = vk::PipelineStageFlagBits2::eTopOfPipe,
		.dstAccessMask = vk::AccessFlagBits2::eMemoryRead,

		.srcQueueFamilyIndex = context.queueFamilies[CommandType::AsyncCompute],
		.dstQueueFamilyIndex = context.queueFamilies[CommandType::Graphics],
		.buffer = particlePositions,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vk::Result dontcare = context.device.waitForFences(*asyncFences[frameID % 2], true, UINT64_MAX);
	context.device.resetFences(*asyncFences[frameID % 2]);

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
	asyncCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0			, sizeof(float)   , (void*)&m_runTime);
	asyncCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, sizeof(float), sizeof(uint32_t), (void*)&frameID);
	asyncCmds.dispatch(PARTICLE_COUNT / 32, 1, 1);

	//Release the buffer to the gfx queue
	asyncCmds.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseComputeBarrier
		}
	);

	CmdBufferEndSubmit(asyncCmds, context.queues[CommandType::AsyncCompute], *asyncFences[frameID % 2]);

	//Aquire the buffer from the compute queue
	context.cmdBuffer.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseComputeBarrier
		}
	);

	context.cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithColourAttachment(context.colourView)
			.WithRenderArea(context.screenRect)
			.Build()
	);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rasterPipeline);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *rasterPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	context.cmdBuffer.pushConstants(*rasterPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&frameID);
	context.cmdBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

	context.cmdBuffer.endRendering();

	//Release the buffer from the gfx queue
	context.cmdBuffer.pipelineBarrier2(
		{
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &releaseGraphicsBarrier
		}
	);

	frameID++;
}
