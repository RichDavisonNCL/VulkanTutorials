/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "PrerecordedCmdListRenderer.h"

using namespace NCL;
using namespace Rendering;

PrerecordedCmdListRenderer::PrerecordedCmdListRenderer(Window& window) : VulkanRenderer(window)	{
	triMesh = (VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh());
	triMesh->UploadToGPU(this);

	shader = VulkanShaderBuilder()
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
		.WithDebugName("Basic Shader!")
	.Build(device);
	 
	BuildPipeline();

	auto buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
		commandPool, vk::CommandBufferLevel::eSecondary, 1));

	recordedBuffer = buffers[0];

	vk::CommandBufferInheritanceInfo inheritance;
	inheritance.setRenderPass(defaultRenderPass);
	vk::CommandBufferBeginInfo bufferBegin = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance);
	recordedBuffer.begin(bufferBegin);
	recordedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());
	recordedBuffer.setViewport(0, 1, &defaultViewport);
	recordedBuffer.setScissor(0, 1, &defaultScissor);
	SubmitDrawCall(triMesh, recordedBuffer);
	recordedBuffer.end();
}

PrerecordedCmdListRenderer::~PrerecordedCmdListRenderer()	{
	delete triMesh;
	delete shader;
}

void PrerecordedCmdListRenderer::BuildPipeline() {
	pipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader)
		.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithRaster(vk::CullModeFlagBits::eNone)
	.Build(device, pipelineCache);
}

void PrerecordedCmdListRenderer::RenderFrame() {
	defaultCmdBuffer.beginRenderPass(defaultBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);
	defaultCmdBuffer.executeCommands(1, &recordedBuffer);
	defaultCmdBuffer.endRenderPass();
}