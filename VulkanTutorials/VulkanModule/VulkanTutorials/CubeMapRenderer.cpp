/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "CubeMapRenderer.h"

using namespace NCL;
using namespace Rendering;

CubeMapRenderer::CubeMapRenderer(Window& window) : VulkanTutorialRenderer(window) {
	quadMesh	= GenerateQuad();
	sphereMesh	= (VulkanMesh*)LoadMesh("Sphere.msh");

	cubemapLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Cubemap Layout")
		.BuildUnique(device);

	cubeTex = VulkanTexture::VulkanCubemapPtrFromFilename(
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	
	
	cubemapDescriptor = BuildUniqueDescriptorSet(cubemapLayout.get());
	UpdateImageDescriptor(cubemapDescriptor.get(), 0, cubeTex->GetDefaultView(), *defaultSampler);

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Camera Position Layout")
		.BuildUnique(device);

	cameraPosDescriptor = BuildUniqueDescriptorSet(cameraPosLayout.get());

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

	skyboxPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(quadMesh->GetVertexSpecification())
		.WithTopology(quadMesh->GetVulkanTopology())
		.WithShaderState(skyboxShader)
		.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithDescriptorSetLayout(cameraLayout.get())
		.WithDescriptorSetLayout(cubemapLayout.get())
		.WithDebugName("CubeMapRenderer Skybox Pipeline")
		.Build(device, pipelineCache);

	objectPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(sphereMesh->GetVertexSpecification())
		.WithTopology(sphereMesh->GetVulkanTopology())
		.WithShaderState(objectShader)
		.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithPushConstant(vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4)))
		.WithDescriptorSetLayout(cameraLayout.get())	//0
		.WithDescriptorSetLayout(cubemapLayout.get())	//1
		.WithDescriptorSetLayout(cameraPosLayout.get()) //2
		.WithDebugName("CubeMapRenderer Object Pipeline")
		.Build(device, pipelineCache);
}

CubeMapRenderer::~CubeMapRenderer() {
	delete quadMesh;
	delete sphereMesh;
}

void CubeMapRenderer::RenderFrame()		 {
	BeginDefaultRenderPass(defaultCmdBuffer);

	//First bit: Draw skybox!
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline.pipeline.get());
	
	UpdateUniformBufferDescriptor(cameraDescriptor.get(), cameraUniform.cameraData, 0);
	UpdateUniformBufferDescriptor(cameraPosDescriptor.get(), camPosUniform, 0);
	
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline.layout.get(), 0, 1, &cameraDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline.layout.get(), 1, 1, &cubemapDescriptor.get(), 0, nullptr);
	SubmitDrawCall(quadMesh, defaultCmdBuffer);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline.pipeline.get());
	defaultCmdBuffer.pushConstants(objectPipeline.layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);
	//
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objectPipeline.layout.get(), 0, 1, &cameraDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objectPipeline.layout.get(), 1, 1, &cubemapDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objectPipeline.layout.get(), 2, 1, &cameraPosDescriptor.get(), 0, nullptr);
	SubmitDrawCall(sphereMesh, defaultCmdBuffer);

	defaultCmdBuffer.endRenderPass();
}

void CubeMapRenderer::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	Vector3 newCamPos = cameraUniform.camera.GetPosition();
	UploadBufferData(camPosUniform, &newCamPos, sizeof(Vector3));
}