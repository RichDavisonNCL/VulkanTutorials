/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "LightingExample.h"

using namespace NCL;
using namespace Rendering;

LightingExample::LightingExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(-20.0f);
	cameraUniform.camera.SetPosition(Vector3(0, 75.0f, 200));

	cubeMesh = LoadMesh("Cube.msh");

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix4::Translation(Vector3(-50, 10, -50)) * Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f));

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform	= Matrix4::Scale(Vector3(100.0f, 1.0f, 100.0f));

	lightingShader = VulkanShaderBuilder()
		.WithVertexBinary("Lighting.vert.spv")
		.WithFragmentBinary("Lighting.frag.spv")
		.WithDebugName("Lighting Shader")
	.BuildUnique(device);

	Light testLight(Vector3(10, 20, -30), 150.0f, Vector4(1, 0.8f, 0.5f, 1));

	lightUniform  = CreateBuffer(sizeof(Light)  , vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	camPosUniform = CreateBuffer(sizeof(Vector3), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	UploadBufferData(lightUniform, &testLight, sizeof(testLight));

	allTextures[0] = VulkanTexture::TextureFromFile("rust_diffuse.png");
	allTextures[1] = VulkanTexture::TextureFromFile("rust_bump.png");
	allTextures[2] = VulkanTexture::TextureFromFile("concrete_diffuse.png");
	allTextures[3] = VulkanTexture::TextureFromFile("concrete_bump.png");
	BuildPipeline();
}

void LightingExample::Update(float dt) {
	UpdateCamera(dt);	
	UploadCameraUniform();
	Vector3 newCamPos = cameraUniform.camera.GetPosition();
	UploadBufferData(camPosUniform, &newCamPos, sizeof(Vector3));
}

void	LightingExample::BuildPipeline() {
	texturesLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device);

	lightLayout = VulkanDescriptorSetLayoutBuilder("Active Light")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device); //Get our camera matrices...

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder("Camera Position")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device); //Get our camera matrices...

	pipeline = VulkanPipelineBuilder("Lighting Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)	
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(*cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(*lightLayout)		//Light is set 1
		.WithDescriptorSetLayout(*texturesLayout)	//Textures are set 2
		.WithDescriptorSetLayout(*cameraPosLayout)	//Cam Pos is set 3
	.Build(device, pipelineCache);

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	floorObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	cameraPosDescriptor		= BuildUniqueDescriptorSet(*cameraPosLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, allTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, allTextures[1]->GetDefaultView(), *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, allTextures[2]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, allTextures[3]->GetDefaultView(), *defaultSampler);

	UpdateBufferDescriptor(*cameraDescriptor	, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
	UpdateBufferDescriptor(*lightDescriptor		, lightUniform, 0, vk::DescriptorType::eUniformBuffer);
	UpdateBufferDescriptor(*cameraPosDescriptor	, camPosUniform, 0, vk::DescriptorType::eUniformBuffer);
}

void LightingExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithDepthAttachment(depthBuffer->GetDefaultView())
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 3, 1, &*cameraPosDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, defaultCmdBuffer, pipeline, 2);
	RenderSingleObject(floorObject	, defaultCmdBuffer, pipeline, 2);

	EndRendering(defaultCmdBuffer);
}