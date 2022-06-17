/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "ShadowmappingExample.h"

using namespace NCL;
using namespace Rendering;

const int SHADOWSIZE = 2048;

ShadowMappingExample::ShadowMappingExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(-40.0f).SetYaw(310).SetPosition(Vector3(-150,120.0f, 240));

	cubeMesh	= LoadMesh("Cube.msh");
	
	shadowFillShader = VulkanShaderBuilder("Shadow Fill Shader")
		.WithVertexBinary("ShadowFill.vert.spv")
		.WithFragmentBinary("ShadowFill.frag.spv")
	.BuildUnique(device);

	shadowUseShader = VulkanShaderBuilder("Shadow Use Shader")
		.WithVertexBinary("ShadowUse.vert.spv")
		.WithFragmentBinary("ShadowUse.frag.spv")
	.BuildUnique(device);

	shadowMap = VulkanTexture::CreateDepthTexture(SHADOWSIZE, SHADOWSIZE, "Shadow Map", false);

	shadowMatUniform = CreateBuffer(sizeof(Matrix4), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	shadowMatrix = (Matrix4*)device.mapMemory(*shadowMatUniform.deviceMem, 0, shadowMatUniform.allocInfo.allocationSize);

	*shadowMatrix = 	//Let's make the shadow casting light look at something!
		Matrix4::Perspective(1.0f, 200.0f, 1.0f, 45.0f) * 
		Matrix4::BuildViewMatrix(Vector3(-100, 50, -100), Vector3(0, 0, 0), Vector3(0, 1, 0));

	shadowMatrixLayout = VulkanDescriptorSetLayoutBuilder("ShadowMatrix")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.BuildUnique(device); //Get our camera matrices...

	shadowMatrixDescriptor = BuildUniqueDescriptorSet(*shadowMatrixLayout);
	UpdateBufferDescriptor(*shadowMatrixDescriptor, shadowMatUniform, 0, vk::DescriptorType::eUniformBuffer);

	shadowViewport	= vk::Viewport(0.0f, (float)SHADOWSIZE, (float)SHADOWSIZE, -(float)SHADOWSIZE, 0.0f, 1.0f);
	shadowScissor	= vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(SHADOWSIZE, SHADOWSIZE));

	BuildMainPipeline();
	BuildShadowPipeline();

	UpdateImageDescriptor(*sceneShadowTexDescriptor, 0, shadowMap->GetDefaultView(), *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	boxObject.mesh			= &*cubeMesh;
	boxObject.transform		= Matrix4::Translation(Vector3(-50, 25, -50));

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform	= Matrix4::Scale(Vector3(100.0f, 1.0f, 100.0f));
}

ShadowMappingExample::~ShadowMappingExample() {
	DestroyBuffer(shadowMatUniform);
}

void ShadowMappingExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);
	boxObject.transform = Matrix4::Translation(Vector3(-50, 30.0f + (sin(runTime) * 30.0f), -50));
}

void	ShadowMappingExample::BuildShadowPipeline() {
	shadowPipeline = VulkanPipelineBuilder("Shadow Map Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowFillShader)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(*shadowMatrixLayout)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithDepthFormat(shadowMap->GetFormat())
	.Build(device, pipelineCache);
}

void	ShadowMappingExample::BuildMainPipeline() {
	diffuseLayout = VulkanDescriptorSetLayoutBuilder("DiffuseTex")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	
		.BuildUnique(device); //And a diffuse texture

	shadowTexLayout = VulkanDescriptorSetLayoutBuilder("ShadowTex")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.BuildUnique(device); //And our shadow map!

	scenePipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowUseShader)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(*cameraLayout)
		.WithDescriptorSetLayout(*shadowMatrixLayout)
		.WithDescriptorSetLayout(*diffuseLayout)
		.WithDescriptorSetLayout(*shadowTexLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build(device, pipelineCache);

	sceneShadowTexDescriptor	= BuildUniqueDescriptorSet(*shadowTexLayout);
	boxObject.objectDescriptorSet		= BuildUniqueDescriptorSet(*diffuseLayout);
	floorObject.objectDescriptorSet		= BuildUniqueDescriptorSet(*diffuseLayout);
}

void ShadowMappingExample::RenderShadowMap() {
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *shadowPipeline.pipeline);

	VulkanDynamicRenderBuilder()
		.WithDepthAttachment(shadowMap->GetDefaultView())
		.WithRenderArea(shadowScissor)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *shadowPipeline.layout, 0, 1, &*shadowMatrixDescriptor, 0, nullptr);
	defaultCmdBuffer.setViewport(0, 1, &shadowViewport);
	defaultCmdBuffer.setScissor(0, 1, &shadowScissor);

	DrawObjects(shadowPipeline);
	EndRendering(defaultCmdBuffer);
}

void ShadowMappingExample::RenderFrame()	{
	RenderShadowMap();	
	RenderScene();	
}

void ShadowMappingExample::RenderScene() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	TransitionDepthToSampler(&*shadowMap, defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *scenePipeline.pipeline);
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 1, 1, &*shadowMatrixDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 3, 1, &*sceneShadowTexDescriptor, 0, nullptr);

	DrawObjects(scenePipeline);

	SubmitDrawCall(*floorObject.mesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);	
	TransitionSamplerToDepth(&*shadowMap, defaultCmdBuffer);
}

void ShadowMappingExample::DrawObjects(VulkanPipeline& toPipeline) {
	defaultCmdBuffer.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&boxObject.transform);
	SubmitDrawCall(*boxObject.mesh, defaultCmdBuffer);

	defaultCmdBuffer.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&floorObject.transform);
	SubmitDrawCall(*floorObject.mesh, defaultCmdBuffer);
}