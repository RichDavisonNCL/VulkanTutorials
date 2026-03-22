/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "CubeMapExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(CubeMapExample)

CubeMapExample::CubeMapExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	cubeTex = LoadCubemap(
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	

	m_camera.SetPosition({ 0, 0, 10 });
	
	camPosUniform = m_memoryManager->CreateBuffer(
		{
			.size = sizeof(Vector3),
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Camera Position"
	);

	skyboxPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_quadMesh->GetVertexInputState())
		.WithTopology(m_quadMesh->GetVulkanTopology())
		.WithShaderBinary("skybox.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("skybox.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
	.Build("CubeMapRenderer Skybox Pipeline");

	objectPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_sphereMesh->GetVertexInputState())
		.WithTopology(m_sphereMesh->GetVulkanTopology())
		.WithShaderBinary("cubemapObject.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("cubemapObject.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("CubeMapRenderer Object Pipeline");

	cubemapDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, objectPipeline.GetSetLayout(1)); //Both use compatible layout
	WriteCombinedImageDescriptor(context.device, *cubemapDescriptor, 0, cubeTex->GetDefaultView(), *m_defaultSampler);

	cameraPosDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, objectPipeline.GetSetLayout(2));

	WriteBufferDescriptor(context.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
	WriteBufferDescriptor(context.device, *cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void CubeMapExample::RenderFrame(float dt)		 {
	//First bit: Draw skybox!
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer m_cmdBuffer = context.cmdBuffer;

	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline);

	Vector3 newCamPos = m_camera.GetPosition();
	camPosUniform.CopyData(&newCamPos, sizeof(Vector3));

	vk::DescriptorSet skyboxSets[] = {
		*m_cameraDescriptor, //Set 0
		*cubemapDescriptor //Set 1
	};

	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 2, skyboxSets, 0, nullptr);
	m_quadMesh->Draw(m_cmdBuffer);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);
	m_cmdBuffer.pushConstants(*objectPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);

	vk::DescriptorSet objectSets[] = {
		*m_cameraDescriptor,		//Set 0
		*cubemapDescriptor,		//Set 1
		*cameraPosDescriptor	//Set 2
	};
	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 0, 3, objectSets, 0, nullptr);
	m_sphereMesh->Draw(m_cmdBuffer);
}