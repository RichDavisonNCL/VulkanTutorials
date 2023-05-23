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
	autoBeginDynamicRendering = true;
}

void LightingExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	camera.SetPitch(-20.0f);
	camera.SetPosition({ 0, 100.0f, 200 });

	cubeMesh = LoadMesh("Cube.msh");

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix4::Translation({ -50, 10, -50 }) * Matrix4::Scale({ 10.0f, 10.0f, 10.0f });

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform = Matrix4::Scale({ 100.0f, 1.0f, 100.0f });

	lightingShader = VulkanShaderBuilder("Lighting Shader")
		.WithVertexBinary("Lighting.vert.spv")
		.WithFragmentBinary("Lighting.frag.spv")
	.Build(GetDevice());

	Light testLight(Vector3(10, 20, -30), 150.0f, Vector4(1, 0.8f, 0.5f, 1));

	lightUniform = VulkanBufferBuilder(sizeof(Light), "Light Uniform")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	camPosUniform = VulkanBufferBuilder(sizeof(Vector3), "Camera Position Uniform")
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	lightUniform.CopyData((void*)&testLight, sizeof(testLight));

	allTextures[0] = VulkanTexture::TextureFromFile(this,  "rust_diffuse.png");
	allTextures[1] = VulkanTexture::TextureFromFile(this,  "rust_bump.png");
	allTextures[2] = VulkanTexture::TextureFromFile(this,  "concrete_diffuse.png");
	allTextures[3] = VulkanTexture::TextureFromFile(this,  "concrete_bump.png");
	BuildPipeline();
}

void LightingExample::Update(float dt) {
	UpdateCamera(dt);	
	UploadCameraUniform();
	Vector3 newCamPos = camera.GetPosition();
	camPosUniform.CopyData((void*)&newCamPos, sizeof(Vector3));
}

void	LightingExample::BuildPipeline() {
	texturesLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	lightLayout = VulkanDescriptorSetLayoutBuilder("Active Light")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice()); //Get our camera matrices...

	cameraPosLayout = VulkanDescriptorSetLayoutBuilder("Camera Position")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice()); //Get our camera matrices...

	pipeline = VulkanPipelineBuilder("Lighting Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)	
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(1, *lightLayout)		//Light is set 1
		.WithDescriptorSetLayout(2, *texturesLayout)	//Textures are set 2
		.WithDescriptorSetLayout(3, *cameraPosLayout)	//Cam Pos is set 3
	.Build(GetDevice());

	lightDescriptor			= BuildUniqueDescriptorSet(*lightLayout);
	boxObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	floorObject.objectDescriptorSet	= BuildUniqueDescriptorSet(*texturesLayout);
	cameraPosDescriptor		= BuildUniqueDescriptorSet(*cameraPosLayout);

	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 0, 0, allTextures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*boxObject.objectDescriptorSet, 1, 0, allTextures[1]->GetDefaultView(), *defaultSampler);

	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 0, 0, allTextures[2]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*floorObject.objectDescriptorSet, 1, 0, allTextures[3]->GetDefaultView(), *defaultSampler);

	UpdateBufferDescriptor(*cameraDescriptor	, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	UpdateBufferDescriptor(*lightDescriptor		, 0, vk::DescriptorType::eUniformBuffer, lightUniform);
	UpdateBufferDescriptor(*cameraPosDescriptor	, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void LightingExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 3, 1, &*cameraPosDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, frameCmds, pipeline, 2);
	RenderSingleObject(floorObject	, frameCmds, pipeline, 2);
}