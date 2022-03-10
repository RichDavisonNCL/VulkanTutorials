/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "MultiViewportExample.h"

using namespace NCL;
using namespace Rendering;

MultiViewportExample::MultiViewportExample(Window& window) : VulkanTutorialRenderer(window) {
	triMesh = MakeSmartMesh(MeshGeometry::GenerateTriangle(new VulkanMesh()));
	triMesh->UploadToGPU(this);

	shader = VulkanShaderBuilder()
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BasicGeometry.frag.spv")
		.WithDebugName("Basic Shader!")
	.BuildUnique(device);

	basicPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(triMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader.get())
		.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithRaster(vk::CullModeFlagBits::eNone)
	.Build(device, pipelineCache);
}

MultiViewportExample::~MultiViewportExample() {
}

void MultiViewportExample::RenderFrame() {
	BeginDefaultRenderPass(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline.pipeline.get());
	for (int i = 0; i < 4; ++i) {
		float xOffset = (float)(i % 2);
		float yOffset = (float)(i / 2);

		float xSize = (float)(windowWidth / 2);
		float ySize = (float)(windowHeight / 2);

		vk::Viewport viewport = vk::Viewport(xSize * xOffset, ySize * yOffset, xSize, ySize, 0.0f, 1.0f);
		defaultCmdBuffer.setViewport(0, 1, &viewport);
		SubmitDrawCall(triMesh.get(), defaultCmdBuffer);
	}
	defaultCmdBuffer.endRenderPass();
}