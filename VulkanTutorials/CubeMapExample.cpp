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

CubeMapExample::CubeMapExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	quadMesh	= GenerateQuad();
	sphereMesh	= LoadMesh("Sphere.msh");

	FrameState const& state = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	cubeTex = LoadCubemap(
		"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
		"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
		"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
		"Cubemap Texture!"
	);	

	camera.SetPosition({ 0, 0, 10 });
	
	camPosUniform = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Vector3), "Camera Position");

	skyboxShader = ShaderBuilder(device)
		.WithVertexBinary("skybox.vert.spv")
		.WithFragmentBinary("skybox.frag.spv")
	.Build("Basic Shader!");

	objectShader = ShaderBuilder(device)
		.WithVertexBinary("cubemapObject.vert.spv")
		.WithFragmentBinary("cubemapObject.frag.spv")
	.Build("Basic Shader!");

	skyboxPipeline = PipelineBuilder(device)
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(quadMesh->GetVulkanTopology())
		.WithShader(skyboxShader)
		.WithColourAttachment(state.colourFormat)
		.WithDepthAttachment(state.depthFormat)
	.Build("CubeMapRenderer Skybox Pipeline");

	objectPipeline = PipelineBuilder(device)
		.WithVertexInputState(sphereMesh->GetVertexInputState())
		.WithTopology(sphereMesh->GetVulkanTopology())
		.WithShader(objectShader)	
		.WithColourAttachment(state.colourFormat)
		.WithDepthAttachment(state.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("CubeMapRenderer Object Pipeline");

	cubemapDescriptor = CreateDescriptorSet(device, pool, objectShader->GetLayout(1)); //Both use compatible layout
	WriteImageDescriptor(device , *cubemapDescriptor, 0, cubeTex->GetDefaultView(), *defaultSampler);

	cameraPosDescriptor = CreateDescriptorSet(device, pool, objectShader->GetLayout(2));

	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	WriteBufferDescriptor(device, *cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void CubeMapExample::RenderFrame(float dt)		 {
	//First bit: Draw skybox!
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline);

	Vector3 newCamPos = camera.GetPosition();
	camPosUniform.CopyData(&newCamPos, sizeof(Vector3));

	vk::DescriptorSet skyboxSets[] = {
		*cameraDescriptor, //Set 0
		*cubemapDescriptor //Set 1
	};

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 2, skyboxSets, 0, nullptr);
	quadMesh->Draw(cmdBuffer);

	//Then draw a 'reflective' sphere!
	Matrix4 objectModelMatrix;
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);
	cmdBuffer.pushConstants(*objectPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&objectModelMatrix);

	vk::DescriptorSet objectSets[] = {
		*cameraDescriptor,		//Set 0
		*cubemapDescriptor,		//Set 1
		*cameraPosDescriptor	//Set 2
	};
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *objectPipeline.layout, 0, 3, objectSets, 0, nullptr);
	sphereMesh->Draw(cmdBuffer);
}