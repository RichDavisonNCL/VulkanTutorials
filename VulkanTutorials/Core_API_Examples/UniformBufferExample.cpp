/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "UniformBufferExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(UniformBufferExample)

UniformBufferExample::UniformBufferExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	cameraData = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Matrix4) * 2,
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Camera Data"
	);

	cameraMemory = (Matrix4*)cameraData.Map();
	camera.SetPosition({ 0,0,10 });

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicUniformBuffer.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicUniformBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.Build("UBO Pipeline");

	descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(0));

	WriteBufferDescriptor(context.device , *descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraData);
}

UniformBufferExample::~UniformBufferExample() {
	cameraData.Unmap();
}

void UniformBufferExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	camera.UpdateCamera(dt);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	UpdateCameraUniform();

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	m_triangleMesh->Draw(context.cmdBuffer);
}

void UniformBufferExample::UpdateCameraUniform() {
	Matrix4 camMatrices[2];
	camMatrices[0] = camera.BuildViewMatrix();
	camMatrices[1] = camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect());
	//'Traditional' method...
	//UpdateUniform(cameraData, (void*)&camMatrices, sizeof(Matrix4) * 2);

	//'Modern' method - just permamap the buffer and transfer the new data directly to GPU
	//Only works with the host visible bit set...
	cameraMemory[0] = camMatrices[0];
	cameraMemory[1] = camMatrices[1];
}