/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicGeometryRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicGeometryRenderer::BasicGeometryRenderer(Window& window) : VulkanRenderer(window)	{
	triMesh = std::shared_ptr<VulkanMesh>((VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh()));
	triMesh->UploadToGPU(this);

	shader = VulkanShaderBuilder()
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
		.WithDebugName("Basic Shader!")
	.BuildUnique(device);

	BuildPipeline();
}

BasicGeometryRenderer::~BasicGeometryRenderer()	{
}

void	BasicGeometryRenderer::BuildPipeline() {
	basicPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader.get())
		//.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithRaster(vk::CullModeFlagBits::eNone)
	.Build(device, pipelineCache);
}

void BasicGeometryRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline.pipeline.get());
	SubmitDrawCall(triMesh.get(), defaultCmdBuffer);
	EndRendering(defaultCmdBuffer);
}