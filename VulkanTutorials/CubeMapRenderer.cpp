/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "CubeMapRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

CubeMapRenderer::CubeMapRenderer(Window& window) : VulkanTutorialRenderer(window) {
}

void CubeMapRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	quadMesh	= GenerateQuad();
	sphereMesh	= LoadMesh("Sphere.msh");

	cubemapLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Cubemap Layout");

	cubeTex = LoadCubemap(
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	

	camera.SetPosition({ 0, 0, 10 });
	
	cubemapDescriptor = BuildUniqueDescriptorSet(*cubemapLayout);
	WriteImageDescriptor(*cubemapDescriptor, 0, 0, cubeTex->GetDefaultView(), *defaultSampler);

	cameraPosLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Camera Position Layout");

	cameraPosDescriptor = BuildUniqueDescriptorSet(*cameraPosLayout);

	camPosUniform = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Vector3), "Camera Position");

	skyboxShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("skybox.vert.spv")
		.WithFragmentBinary("skybox.frag.spv")
	.Build("Basic Shader!");

	objectShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("cubemapObject.vert.spv")
		.WithFragmentBinary("cubemapObject.frag.spv")
	.Build("Basic Shader!");

	skyboxPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(quadMesh->GetVulkanTopology())
		.WithShader(skyboxShader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())	
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*cubemapLayout)
	.Build("CubeMapRenderer Skybox Pipeline");

	objectPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(sphereMesh->GetVulkanTopology())
		.WithShader(objectShader)	
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(0,*cameraLayout)
		.WithDescriptorSetLayout(1,*cubemapLayout)
		.WithDescriptorSetLayout(2,*cameraPosLayout)
	.Build("CubeMapRenderer Object Pipeline");

	WriteBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	WriteBufferDescriptor(*cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void CubeMapRenderer::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	Vector3 newCamPos = camera.GetPosition();
	camPosUniform.CopyData(&newCamPos, sizeof(Vector3));
}

void CubeMapRenderer::RenderFrame()		 {
	//First bit: Draw skybox!
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline);

	vk::DescriptorSet skyboxSets[] = {
		*cameraDescriptor, //Set 0
		*cubemapDescriptor //Set 1
	};

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 2, skyboxSets, 0, nullptr);
	DrawMesh(frameCmds, *quadMesh);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);
	frameCmds.pushConstants(*objectPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);

	vk::DescriptorSet objectSets[] = {
		*cameraDescriptor,		//Set 0
		*cubemapDescriptor,		//Set 1
		*cameraPosDescriptor	//Set 2
	};
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 0, 3, objectSets, 0, nullptr);
	DrawMesh(frameCmds, *sphereMesh);
}