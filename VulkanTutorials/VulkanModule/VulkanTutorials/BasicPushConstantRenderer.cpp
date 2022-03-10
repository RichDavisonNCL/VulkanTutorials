/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicPushConstantRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicPushConstantRenderer::BasicPushConstantRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	positionUniform = Vector3(0.0, 0.0, 0.0f);
	colourUniform	= Vector4(1, 1, 0, 1);

	triMesh = (VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh());
	triMesh->UploadToGPU(this);

	basicShader = VulkanShaderBuilder()
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("BasicPushConstant.frag.spv")
		.WithDebugName("Testing push constants!")
	.Build(device);

	BuildPipeline();
}

BasicPushConstantRenderer::~BasicPushConstantRenderer()	{
	delete basicShader;
	delete triMesh;
}

void	BasicPushConstantRenderer::BuildPipeline() {
	basicPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(basicShader)
		.WithPass(defaultRenderPass)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform))
		.WithPushConstant(vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform))
		.WithDebugName("BasicPushConstantRenderer Pipeline")
	.Build(device, pipelineCache);
}

void BasicPushConstantRenderer::RenderFrame() {
	positionUniform.x = sin(runTime);
	BeginDefaultRenderPass(defaultCmdBuffer);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline.pipeline.get());

	defaultCmdBuffer.pushConstants(basicPipeline.layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(positionUniform), (void*)&positionUniform);
	defaultCmdBuffer.pushConstants(basicPipeline.layout.get(), vk::ShaderStageFlagBits::eFragment, sizeof(Vector4), sizeof(colourUniform), (void*)&colourUniform);

	SubmitDrawCall(triMesh, defaultCmdBuffer);
	defaultCmdBuffer.endRenderPass();
}