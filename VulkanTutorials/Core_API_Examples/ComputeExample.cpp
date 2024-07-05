/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "ComputeExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(ComputeExample)

const int PARTICLE_COUNT = 32 * 100;

ComputeExample::ComputeExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	vkInit.	autoBeginDynamicRendering = false;

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& state = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	particlePositions = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(Vector4) * PARTICLE_COUNT, "Particles!");

	dataLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Data"); //Get our camera matrices...

	rasterShader = ShaderBuilder(device)
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("BasicCompute.frag.spv")
		.Build("Shader using compute data!");

	basicPipeline = PipelineBuilder(device)
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithShader(rasterShader)
		.WithColourAttachment(state.colourFormat)
		.WithDescriptorSetLayout(0, *dataLayout)
		.Build("Raster Pipeline");

	computeShader = UniqueVulkanCompute(new VulkanCompute(device, "BasicCompute.comp.spv"));

	computePipeline = ComputePipelineBuilder(device)
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.Build("Compute Pipeline");
	
	bufferDescriptor = CreateDescriptorSet(device , pool, *dataLayout);
	WriteBufferDescriptor(device , *bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
}

void ComputeExample::RenderFrame(float dt) {
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	cmdBuffer.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);

	cmdBuffer.dispatch(PARTICLE_COUNT / 32, 1, 1);

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, 
		vk::PipelineStageFlagBits::eVertexShader, 
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	cmdBuffer.beginRendering(
		DynamicRenderBuilder()
			.WithColourAttachment(frameState.colourView)
			.WithRenderArea(frameState.defaultScreenRect)
			.Build()
	);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	cmdBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

	cmdBuffer.endRendering();
}