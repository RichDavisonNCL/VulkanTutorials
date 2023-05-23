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
}

void CubeMapRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	quadMesh	= GenerateQuad();
	sphereMesh	= LoadMesh("Sphere.msh");

	cubemapLayout = VulkanDescriptorSetLayoutBuilder("Cubemap Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	cubeTex = VulkanTexture::CubemapFromFiles(this,
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	

	camera.SetPosition({ 0, 0, 10 });
	
	cubemapDescriptor = BuildUniqueDescriptorSet(*cubemapLayout);
	UpdateImageDescriptor(*cubemapDescriptor, 0, 0, cubeTex->GetDefaultView(), *defaultSampler);

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder("Camera Position Layout")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	cameraPosDescriptor = BuildUniqueDescriptorSet(*cameraPosLayout);

	camPosUniform = VulkanBufferBuilder(sizeof(Vector3), "Camera Position")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	skyboxShader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("skybox.vert.spv")
		.WithFragmentBinary("skybox.frag.spv")
	.Build(GetDevice());

	objectShader = VulkanShaderBuilder("Basic Shader!")
		.WithVertexBinary("cubemapObject.vert.spv")
		.WithFragmentBinary("cubemapObject.frag.spv")
	.Build(GetDevice());

	skyboxPipeline = VulkanPipelineBuilder("CubeMapRenderer Skybox Pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(quadMesh->GetVulkanTopology())
		.WithShader(skyboxShader)
		.WithDepthState(vk::CompareOp::eAlways, false, false, false)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*cubemapLayout)
	.Build(GetDevice());

	objectPipeline = VulkanPipelineBuilder("CubeMapRenderer Object Pipeline")
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(sphereMesh->GetVulkanTopology())
		.WithShader(objectShader)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*cubemapLayout)
		.WithDescriptorSetLayout(2,*cameraPosLayout)
	.Build(GetDevice());

	UpdateBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	UpdateBufferDescriptor(*cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void CubeMapRenderer::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	Vector3 newCamPos = camera.GetPosition();
	camPosUniform.CopyData(&newCamPos, sizeof(Vector3));
}

void CubeMapRenderer::RenderFrame()		 {
	//First bit: Draw skybox!
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 1, 1, &*cubemapDescriptor, 0, nullptr);
	SubmitDrawCall(frameCmds, *quadMesh);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);
	frameCmds.pushConstants(*objectPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);
	//
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 1, 1, &*cubemapDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 2, 1, &*cameraPosDescriptor, 0, nullptr);
	SubmitDrawCall(frameCmds, *sphereMesh);
}