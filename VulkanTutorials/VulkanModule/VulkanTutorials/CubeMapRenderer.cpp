/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "CubeMapRenderer.h"

using namespace NCL;
using namespace Rendering;

CubeMapRenderer::CubeMapRenderer(Window& window) : VulkanTutorialRenderer(window) {
	quadMesh	= GenerateQuad();
	sphereMesh	= LoadMesh("Sphere.msh");

	cubemapLayout = VulkanDescriptorSetLayoutBuilder("Cubemap Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.BuildUnique(device);

	cubeTex = VulkanTexture::VulkanCubemapFromFiles(
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	

	cameraUniform.camera.SetPosition(Vector3(0, 0, 10));
	
	cubemapDescriptor = BuildUniqueDescriptorSet(*cubemapLayout);
	UpdateImageDescriptor(*cubemapDescriptor, 0, cubeTex->GetDefaultView(), *defaultSampler);

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder("Camera Position Layout")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
		.BuildUnique(device);

	cameraPosDescriptor = BuildUniqueDescriptorSet(*cameraPosLayout);

	camPosUniform = CreateBuffer(sizeof(Vector3), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	skyboxShader = VulkanShaderBuilder()
		.WithVertexBinary("skybox.vert.spv")
		.WithFragmentBinary("skybox.frag.spv")
		.WithDebugName("Basic Shader!")
	.BuildUnique(device);

	objectShader = VulkanShaderBuilder()
		.WithVertexBinary("cubemapObject.vert.spv")
		.WithFragmentBinary("cubemapObject.frag.spv")
		.WithDebugName("Basic Shader!")
	.BuildUnique(device);

	skyboxPipeline = VulkanPipelineBuilder("CubeMapRenderer Skybox Pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(quadMesh->GetVulkanTopology())
		.WithShader(skyboxShader)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(*cameraLayout)
		.WithDescriptorSetLayout(*cubemapLayout)
	.Build(device, pipelineCache);

	objectPipeline = VulkanPipelineBuilder("CubeMapRenderer Object Pipeline")
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(sphereMesh->GetVulkanTopology())
		.WithShader(objectShader)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(*cameraLayout)	//0
		.WithDescriptorSetLayout(*cubemapLayout)	//1
		.WithDescriptorSetLayout(*cameraPosLayout) //2
	.Build(device, pipelineCache);

	UpdateBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
	UpdateBufferDescriptor(*cameraPosDescriptor, camPosUniform, 0, vk::DescriptorType::eUniformBuffer);
}

void CubeMapRenderer::RenderFrame()		 {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	//First bit: Draw skybox!
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 1, 1, &*cubemapDescriptor, 0, nullptr);
	SubmitDrawCall(*quadMesh, defaultCmdBuffer);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *objectPipeline.pipeline);
	defaultCmdBuffer.pushConstants(*objectPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);
	//
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 1, 1, &*cubemapDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 2, 1, &*cameraPosDescriptor, 0, nullptr);
	SubmitDrawCall(*sphereMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}

void CubeMapRenderer::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	Vector3 newCamPos = cameraUniform.camera.GetPosition();
	UploadBufferData(camPosUniform, &newCamPos, sizeof(Vector3));
}