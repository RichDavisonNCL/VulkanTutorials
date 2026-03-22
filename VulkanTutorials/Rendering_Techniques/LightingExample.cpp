/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "LightingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(LightingExample)

LightingExample::LightingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.autoBeginDynamicRendering = true;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	m_camera.SetPitch(-20.0f).SetPosition({ 0, 100.0f, 200 });

	boxObject.mesh		= &*m_cubeMesh;
	boxObject.transform = Matrix::Translation(Vector3{ -20, 10, -20 }) * Matrix::Scale(Vector3{ 10.0f, 10.0f, 10.0f });

	floorObject.mesh		= &*m_cubeMesh;
	floorObject.transform = Matrix::Scale(Vector3{ 100.0f, 1.0f, 100.0f });

	Light testLight(Vector3(0, 20, -30), 150.0f, Vector4(1, 0.8f, 0.5f, 1));

	lightUniform = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Light),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Light Uniform"
	);

	camPosUniform = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(Vector3),
			.usage = vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Camera Position Uniform"
	);

	lightUniform.CopyData((void*)&testLight, sizeof(testLight));

	allTextures[0] = LoadTexture("rust_diffuse.png");
	allTextures[1] = LoadTexture("rust_bump.png");
	allTextures[2] = LoadTexture("concrete_diffuse.png");
	allTextures[3] = LoadTexture("concrete_bump.png");

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("Lighting.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("Lighting.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.Build("Lighting Pipeline");

	lightDescriptor				= CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(1));
	boxObject.descriptorSet		= CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(2));
	floorObject.descriptorSet	= CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(2));
	cameraPosDescriptor			= CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(3));

	WriteCombinedImageDescriptor(context.device, *boxObject.descriptorSet, 0, allTextures[0]->GetDefaultView(), *m_defaultSampler);
	WriteCombinedImageDescriptor(context.device, *boxObject.descriptorSet, 1, allTextures[1]->GetDefaultView(), *m_defaultSampler);

	WriteCombinedImageDescriptor(context.device, *floorObject.descriptorSet, 0, allTextures[2]->GetDefaultView(), *m_defaultSampler);
	WriteCombinedImageDescriptor(context.device, *floorObject.descriptorSet, 1, allTextures[3]->GetDefaultView(), *m_defaultSampler);

	WriteBufferDescriptor(context.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
	WriteBufferDescriptor(context.device, *lightDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightUniform);
	WriteBufferDescriptor(context.device, *cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void LightingExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	Vector3 newCamPos = m_camera.GetPosition();
	camPosUniform.CopyData((void*)&newCamPos, sizeof(Vector3));

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 3, 1, &*cameraPosDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, context.cmdBuffer, pipeline, 2);
	RenderSingleObject(floorObject	, context.cmdBuffer, pipeline, 2);
}