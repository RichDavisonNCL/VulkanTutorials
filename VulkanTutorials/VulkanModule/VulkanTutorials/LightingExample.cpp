/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "LightingExample.h"

using namespace NCL;
using namespace Rendering;

LightingExample::LightingExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(-20.0f);
	cameraUniform.camera.SetPosition(Vector3(0, 75.0f, 200));

	cubeMesh = MakeSmartMesh(LoadMesh("Cube.msh"));

	boxObject.mesh		= cubeMesh.get();
	boxObject.transform = Matrix4::Translation(Vector3(-50, 10, -50)) * Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f));

	floorObject.mesh		= cubeMesh.get();
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

	allTextures[0] = VulkanTexture::TexturePtrFromFilename("rust_diffuse.png");
	allTextures[1] = VulkanTexture::TexturePtrFromFilename("rust_bump.png");
	allTextures[2] = VulkanTexture::TexturePtrFromFilename("concrete_diffuse.png");
	allTextures[3] = VulkanTexture::TexturePtrFromFilename("concrete_bump.png");
	BuildPipeline();
}

LightingExample::~LightingExample() {
}

void LightingExample::Update(float dt) {
	UpdateCamera(dt);	
	UploadCameraUniform();
	Vector3 newCamPos = cameraUniform.camera.GetPosition();
	UploadBufferData(camPosUniform, &newCamPos, sizeof(Vector3));
}

void	LightingExample::BuildPipeline() {
	vk::PushConstantRange modelMatrixConstant = vk::PushConstantRange()
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setOffset(0) //we can only have one push constant per stage...
		.setSize(sizeof(Matrix4)); //We need a model matrix as usual, but also the light matrix!

	texturesLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Object Textures")
	.BuildUnique(device);

	lightLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Active Light")
	.BuildUnique(device); //Get our camera matrices...

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Camera Position")
	.BuildUnique(device); //Get our camera matrices...

	pipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(cubeMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(lightingShader.get())
		.WithPass(defaultRenderPass)		
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithPushConstant(modelMatrixConstant)
		.WithDescriptorSetLayout(cameraLayout.get())		//Camera is set 0
		.WithDescriptorSetLayout(lightLayout.get())		//Light is set 1
		.WithDescriptorSetLayout(texturesLayout.get())	//Textures are set 2
		.WithDescriptorSetLayout(cameraPosLayout.get())	//Cam Pos is set 3
		.WithDebugName("Lighting Pipeline")
	.Build(device, pipelineCache);

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	floorObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	cameraPosDescriptor		= BuildUniqueDescriptorSet(*cameraPosLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, allTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, allTextures[1]->GetDefaultView(), *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, allTextures[2]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, allTextures[3]->GetDefaultView(), *defaultSampler);
}

void LightingExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	//defaultCmdBuffer.beginRenderPass(defaultBeginInfo, vk::SubpassContents::eInline);

	UpdateUniformBufferDescriptor(cameraDescriptor.get()	, cameraUniform.cameraData, 0);
	UpdateUniformBufferDescriptor(lightDescriptor.get()	, lightUniform, 0);
	UpdateUniformBufferDescriptor(cameraPosDescriptor.get(), camPosUniform, 0);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 0, 1, &cameraDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 1, 1, &lightDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 3, 1, &cameraPosDescriptor.get(), 0, nullptr);

	RenderSingleObject(boxObject	, defaultCmdBuffer, pipeline, 2);
	RenderSingleObject(floorObject	, defaultCmdBuffer, pipeline, 2);

//	defaultCmdBuffer.endRenderPass();
	EndRendering(defaultCmdBuffer);
}