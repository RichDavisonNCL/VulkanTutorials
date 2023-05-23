/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicUniformBufferRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicUniformBufferRenderer::BasicUniformBufferRenderer(Window& window) : VulkanTutorialRenderer(window)	{

}

BasicUniformBufferRenderer::~BasicUniformBufferRenderer() {
	cameraData.Unmap();
}

void BasicUniformBufferRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	camera	= Camera::BuildPerspectiveCamera(Vector3(0, 0, 10.0f), 0, 0, 45.0f, 1.0f, 100.0f);

	triMesh = GenerateTriangle();

	shader = VulkanShaderBuilder("Basic Uniform Buffer Usage!")
		.WithVertexBinary("BasicUniformBuffer.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build(GetDevice());

	cameraData = VulkanBufferBuilder(sizeof(Matrix4) * 2, "Camera Data")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	cameraMemory = (Matrix4*)cameraData.Map();
	
	matrixLayout = VulkanDescriptorSetLayoutBuilder("Matrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(GetDevice());

	pipeline = VulkanPipelineBuilder("UBOPipeline")
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *matrixLayout)
		.Build(GetDevice());

	descriptorSet = BuildUniqueDescriptorSet(*matrixLayout);

	UpdateBufferDescriptor(*descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraData);
}

void BasicUniformBufferRenderer::Update(float dt) {
	camera.UpdateCamera(dt);
}

void BasicUniformBufferRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform();

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	SubmitDrawCall(frameCmds, *triMesh);
}

void BasicUniformBufferRenderer::UpdateCameraUniform() {
	Matrix4 camMatrices[2];
	camMatrices[0] = camera.BuildViewMatrix();
	camMatrices[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());
	//'Traditional' method...
	//UpdateUniform(cameraData, (void*)&camMatrices, sizeof(Matrix4) * 2);

	//'Modern' method - just permamap the buffer and transfer the new data directly to GPU
	//Only works with the host visible bit set...
	cameraMemory[0] = camMatrices[0];
	cameraMemory[1] = camMatrices[1];
}