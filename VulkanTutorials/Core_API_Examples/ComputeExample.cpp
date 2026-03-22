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

const int PARTICLE_COUNT = 32 * 10;

ComputeExample::ComputeExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	m_vkInit.autoBeginDynamicRendering = false;

	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	particlePositions = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Vector4) * PARTICLE_COUNT,
			.usage	= vk::BufferUsageFlagBits::eStorageBuffer
		},
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		"Particles!"
	);

	sharedLayout = DescriptorSetLayoutBuilder(context.device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Data"); //Get our m_camera matrices...

	basicPipeline = PipelineBuilder(context.device)
		.WithTopology(vk::PrimitiveTopology::ePointList)
		.WithShaderBinary("BasicCompute.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicCompute.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithDescriptorSetLayout(0, *sharedLayout)
		.Build("Raster Pipeline");

	computePipeline = ComputePipelineBuilder(context.device)
		.WithDescriptorSetLayout(0, *sharedLayout)
		.WithShaderBinary("BasicCompute.comp.spv")
		.Build("Compute Pipeline");
	
	bufferDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, *sharedLayout);
	WriteBufferDescriptor(context.device, *bufferDescriptor, 0, vk::DescriptorType::eStorageBuffer, particlePositions);
}

void ComputeExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	context.cmdBuffer.pushConstants(*computePipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), (void*)&m_runTime);

	context.cmdBuffer.dispatch(PARTICLE_COUNT / 32, 1, 1);

	context.cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, 
		vk::PipelineStageFlagBits::eVertexShader, 
		vk::DependencyFlags(), 0, nullptr, 0, nullptr, 0, nullptr
	);

	m_renderer->BeginRenderToScreen(context.cmdBuffer);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *basicPipeline.layout, 0, 1, &*bufferDescriptor, 0, nullptr);
	context.cmdBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

	context.cmdBuffer.endRendering();
}