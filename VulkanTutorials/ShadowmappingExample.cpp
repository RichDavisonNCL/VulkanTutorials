/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "ShadowmappingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int SHADOWSIZE = 2048;

ShadowMappingExample::ShadowMappingExample(Window& window) : VulkanTutorialRenderer(window) {
	autoBeginDynamicRendering = false;
}

void ShadowMappingExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	camera.SetPitch(-45.0f).SetYaw(310).SetPosition({ -50, 60.0f, 50 });

	cubeMesh	= LoadMesh("Cube.msh");
	
	shadowFillShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("ShadowFill.vert.spv")
		.WithFragmentBinary("ShadowFill.frag.spv")
	.Build("Shadow Fill Shader");

	shadowUseShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("ShadowUse.vert.spv")
		.WithFragmentBinary("ShadowUse.frag.spv")
	.Build("Shadow Use Shader");

	shadowMap = TextureBuilder(GetDevice(), GetMemoryAllocator())
		.WithPool(GetCommandPool(CommandBuffer::Graphics))
		.WithQueue(GetQueue(CommandBuffer::Graphics))
		.WithDimension(SHADOWSIZE, SHADOWSIZE)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithMips(false)
	.Build("Shadowmap");

	shadowMatUniform = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4), "Shadow Matrix");

	shadowMatrix = (Matrix4*)shadowMatUniform.Map(); 
	*shadowMatrix = 	//Let's make the shadow casting light look at something!
		Matrix4::Perspective(1.0f, 200.0f, 1.0f, 45.0f) * 
		Matrix4::BuildViewMatrix(Vector3(-50, 50, -50), Vector3(0, 0, 0), Vector3(0, 1, 0));
	shadowMatUniform.Unmap();

	shadowMatrixLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build("ShadowMatrix"); //Get our camera matrices...

	shadowMatrixDescriptor = BuildUniqueDescriptorSet(*shadowMatrixLayout);
	WriteBufferDescriptor(*shadowMatrixDescriptor, 0, vk::DescriptorType::eUniformBuffer, shadowMatUniform);

	BuildMainPipeline();
	BuildShadowPipeline();

	WriteImageDescriptor(*sceneShadowTexDescriptor, 0, 0, shadowMap->GetDefaultView(), *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);

	sceneObjects[0].mesh = &*cubeMesh;
	sceneObjects[0].transform = Matrix4::Translation({ -20, 25, -20 });

	sceneObjects[1].mesh		= &*cubeMesh;
	sceneObjects[1].transform = Matrix4::Scale({ 40.0f, 1.0f, 40.0f });
}

void	ShadowMappingExample::BuildShadowPipeline() {
	shadowPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowFillShader)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(0, *shadowMatrixLayout)
		.WithDepthAttachment(shadowMap->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
	.Build("Shadow Map Fill Pipeline");
}

void	ShadowMappingExample::BuildMainPipeline() {
	diffuseLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	
		.Build("DiffuseTex"); //And a diffuse texture

	shadowTexLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.Build("ShadowTex"); //And our shadow map!

	scenePipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shadowUseShader)
		.WithDepthAttachment(depthBuffer->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)
		.WithDescriptorSetLayout(1, *shadowMatrixLayout)
		.WithDescriptorSetLayout(2, *diffuseLayout)
		.WithDescriptorSetLayout(3, *shadowTexLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
	.Build("Main Scene Draw Pipeline");

	sceneShadowTexDescriptor	= BuildUniqueDescriptorSet(*shadowTexLayout);

	for (auto& o : sceneObjects) {
		o.descriptorSet = BuildUniqueDescriptorSet(*diffuseLayout);
	}
}

void ShadowMappingExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);
	sceneObjects[0].transform = Matrix4::Translation({-20, 10.0f + (sin(runTime) * 10.0f), -20});
}

void ShadowMappingExample::RenderFrame()	{
	RenderShadowMap();	
	RenderScene();	
}

void ShadowMappingExample::DrawObjects(VulkanPipeline& toPipeline) {
	for (auto& o : sceneObjects) {
		frameCmds.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&o.transform);
		DrawMesh(frameCmds, *o.mesh);
	}
}

void ShadowMappingExample::RenderShadowMap() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline);

	vk::Viewport shadowViewport = vk::Viewport(0.0f, (float)SHADOWSIZE, (float)SHADOWSIZE, -(float)SHADOWSIZE, 0.0f, 1.0f);
	vk::Rect2D	 shadowScissor	= vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(SHADOWSIZE, SHADOWSIZE));

	DynamicRenderBuilder()
		.WithDepthAttachment(shadowMap->GetDefaultView())
		.WithRenderArea(shadowScissor)
	.BeginRendering(frameCmds);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *shadowPipeline.layout, 0, 1, &*shadowMatrixDescriptor, 0, nullptr);
	frameCmds.setViewport(0, 1, &shadowViewport);
	frameCmds.setScissor(0, 1, &shadowScissor);

	DrawObjects(shadowPipeline);
	frameCmds.endRendering();
}

void ShadowMappingExample::RenderScene() {
	TransitionDepthToSampler(frameCmds, *shadowMap);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, scenePipeline);
	BeginDefaultRendering(frameCmds);

	frameCmds.setViewport(0, 1, &defaultViewport);
	frameCmds.setScissor(0, 1, &defaultScissor);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 1, 1, &*shadowMatrixDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 3, 1, &*sceneShadowTexDescriptor, 0, nullptr);

	DrawObjects(scenePipeline);

	frameCmds.endRendering();
	TransitionSamplerToDepth(frameCmds, *shadowMap);
}