/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicComputeUsage.h"

using namespace NCL;
using namespace Rendering;

const int PARTICLE_COUNT = 320 * 10;

BasicComputeUsage::BasicComputeUsage(Window& window) : VulkanTutorialRenderer(window)	{
	autoBeginDynamicRendering = false;
}

void BasicComputeUsage::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	particlePositions = VulkanBufferBuilder(sizeof(Vector4) * PARTICLE_COUNT, "Particles!")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(GetDevice(), GetMemoryAllocator());

	dataLayout = VulkanDescriptorSetLayoutBuilder("Compute Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
	.Build(GetDevice()); //Get our camera matrices...

	bufferDescriptor = BuildUniqueDescriptorSet(*dataLayout);
	UpdateBufferDescriptor(*bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);

	BuildComputePipeline();
	BuildRasterPipeline();
}

void	BasicComputeUsage::BuildRasterPipeline() {
	rasterShader = VulkanShaderBuilder("Shader using compute data!")
		.WithVertexBinary("BasicCompute.vert.spv")
		.WithFragmentBinary("BasicCompute.frag.spv")
	.Build(GetDevice());

	basicPipeline = VulkanPipelineBuilder("Raster Pipeline")
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithShader(rasterShader)
		.WithDepthFormat(depthBuffer->GetFormat())
	.Build(GetDevice());
}

void	BasicComputeUsage::BuildComputePipeline() {
	computeShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "BasicCompute.comp.spv"));

	computePipeline = VulkanComputePipelineBuilder("Compute Pipeline")
		.WithShader(computeShader)
		.WithDescriptorSetLayout(0, *dataLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float))
	.Build(GetDevice());
}

void BasicComputeUsage::RenderFrame() {
	//Compute goes outside of a render pass...
	frameCmds.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	frameCmds.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&runTime);

	Vulkan::DispatchCompute(frameCmds, PARTICLE_COUNT / 32, 1, 1);

	frameCmds.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, 
		vk::PipelineStageFlagBits::eVertexShader, 
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(GetCurrentSwapView())
		.WithRenderArea(defaultScreenRect)
		.BeginRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	frameCmds.draw(PARTICLE_COUNT, 1, 0, 0);

	EndRendering(frameCmds);
}