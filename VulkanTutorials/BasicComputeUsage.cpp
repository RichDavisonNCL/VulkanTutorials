/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicComputeUsage.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int PARTICLE_COUNT = 320 * 10;

BasicComputeUsage::BasicComputeUsage(Window& window) : VulkanTutorialRenderer(window)	{
	autoBeginDynamicRendering = false;
}

void BasicComputeUsage::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	particlePositions = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(sizeof(Vector4) * PARTICLE_COUNT, "Particles!");

	dataLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
	.Build("Compute Data"); //Get our camera matrices...

	bufferDescriptor = BuildUniqueDescriptorSet(*dataLayout);
	WriteBufferDescriptor(*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);

	BuildComputePipeline();
	BuildRasterPipeline();
}

void	BasicComputeUsage::BuildRasterPipeline() {
	rasterShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("BasicCompute.frag.spv")
	.Build("Shader using compute data!");

	basicPipeline = PipelineBuilder(GetDevice())
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.WithColourAttachment(GetSurfaceFormat())
	.Build("Raster Pipeline");
}

void	BasicComputeUsage::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "BasicCompute.comp.spv"));

	computePipeline = ComputePipelineBuilder(GetDevice())
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float))
	.Build("Compute Pipeline");
}

void BasicComputeUsage::RenderFrame() {
	//Compute goes outside of a render pass...
	frameCmds.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	frameCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);

	frameCmds.dispatch(PARTICLE_COUNT / 32, 1, 1);

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
	frameCmds.draw(PARTICLE_COUNT, 1, 0, 0);

	frameCmds.endRendering();
}