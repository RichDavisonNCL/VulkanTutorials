/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "ShadowmappingExample.h"

using namespace NCL;
using namespace Rendering;

const int SHADOWSIZE = 2048;

ShadowMappingExample::ShadowMappingExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(-20.0f).SetPosition(Vector3(0,75.0f, 200));

	cubeMesh	= LoadMesh("Cube.msh");
	
	shadowFillShader = VulkanShaderBuilder()
		.WithVertexBinary("ShadowFill.vert.spv")
		.WithFragmentBinary("ShadowFill.frag.spv")
		.WithDebugName("Shadow Fill Shader")
		.BuildUnique(device);

	shadowUseShader = VulkanShaderBuilder()
		.WithVertexBinary("ShadowUse.vert.spv")
		.WithFragmentBinary("ShadowUse.frag.spv")
		.WithDebugName("Shadow Use Shader")
		.BuildUnique(device);

	shadowBuffer = BuildFrameBuffer(shadowPass, SHADOWSIZE, SHADOWSIZE, 0, true, false, false);//Note, no colour buffers!

	shadowUniform = CreateBuffer(sizeof(Matrix4), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	shadowMatrix = (Matrix4*)device.mapMemory(shadowUniform.deviceMem.get(), 0, shadowUniform.allocInfo.allocationSize);

	*shadowMatrix = 	//Let's make the shadow casting light look at something!
		Matrix4::Perspective(1.0f, 200.0f, 1.0f, 45.0f) * 
		Matrix4::BuildViewMatrix(Vector3(-100, 50, -100), Vector3(0, 0, 0), Vector3(0, 1, 0));

	shadowMatrixLayout = VulkanDescriptorSetLayoutBuilder()
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithDebugName("ShadowMatrix")
		.BuildUnique(device); //Get our camera matrices...

	shadowMatrixDescriptor = BuildUniqueDescriptorSet(shadowMatrixLayout.get());
	UpdateUniformBufferDescriptor(shadowMatrixDescriptor.get(), shadowUniform, 0);

	shadowViewport	= vk::Viewport(0.0f, (float)SHADOWSIZE, (float)SHADOWSIZE, -(float)SHADOWSIZE, 0.0f, 1.0f);
	shadowScissor	= vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(SHADOWSIZE, SHADOWSIZE));

	BuildMainPipeline();
	BuildShadowPipeline();

	boxObject.mesh			= cubeMesh;
	boxObject.transform		= Matrix4::Translation(Vector3(-50, 25, -50));

	floorObject.mesh		= cubeMesh;
	floorObject.transform	= Matrix4::Scale(Vector3(100.0f, 1.0f, 100.0f));
}

ShadowMappingExample::~ShadowMappingExample() {
	delete cubeMesh;
	DestroyBuffer(shadowUniform);
	device.destroyRenderPass(shadowPass);
}

void ShadowMappingExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);
	boxObject.transform = Matrix4::Translation(Vector3(-50, 30.0f + (sin(runTime) * 30.0f), -50));
}

void	ShadowMappingExample::BuildShadowPipeline() {
	shadowPipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(cubeMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shadowFillShader)
		.WithPass(shadowPass)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDescriptorSetLayout(shadowMatrixLayout.get())
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithDepthStencilFormat(shadowBuffer.depthAttachment->GetFormat())
		.WithDebugName("Shadow Map Pipeline")
	.Build(device, pipelineCache);
}

void	ShadowMappingExample::BuildMainPipeline() {
	diffuseLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)	
		.WithDebugName("DiffuseTex")
		.BuildUnique(device); //And a diffuse texture

	shadowTexLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("ShadowTex")
		.BuildUnique(device); //And our shadow map!

	scenePipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(cubeMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shadowUseShader)
		.WithPass(defaultRenderPass)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(cameraLayout.get())
		.WithDescriptorSetLayout(shadowMatrixLayout.get())
		.WithDescriptorSetLayout(diffuseLayout.get())
		.WithDescriptorSetLayout(shadowTexLayout.get())
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithDebugName("Main Scene Pipeline")
	.Build(device, pipelineCache);

	sceneShadowTexDescriptor	= BuildUniqueDescriptorSet(shadowTexLayout.get());
	boxObject.objectDescriptorSet		= BuildUniqueDescriptorSet(diffuseLayout.get());
	floorObject.objectDescriptorSet		= BuildUniqueDescriptorSet(diffuseLayout.get());
}

void ShadowMappingExample::RenderShadowMap() {
	ImageTransitionBarrier(defaultCmdBuffer, shadowBuffer.depthAttachment.get(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageAspectFlagBits::eDepth,
		vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eEarlyFragmentTests);

	vk::ClearValue shadowClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

	vk::RenderPassBeginInfo passBegin = vk::RenderPassBeginInfo()//This first pass will render our scene into a framebuffer
		.setRenderPass(shadowPass)
		.setFramebuffer(shadowBuffer.frameBuffer.get())
		.setRenderArea(shadowScissor)
		.setClearValueCount(1)
		.setPClearValues(&shadowClearValue);

	defaultCmdBuffer.beginRenderPass(passBegin, vk::SubpassContents::eInline);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline.pipeline.get());
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, shadowPipeline.layout.get(), 0, 1, &shadowMatrixDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.setViewport(0, 1, &shadowViewport);
	defaultCmdBuffer.setScissor(0, 1, &shadowScissor);

	DrawObjects(shadowPipeline);

	defaultCmdBuffer.endRenderPass();

	ImageTransitionBarrier(defaultCmdBuffer, shadowBuffer.depthAttachment.get(),
		vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eDepth,
		vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader);
}

void ShadowMappingExample::RenderScene() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	UpdateImageDescriptor(*sceneShadowTexDescriptor, 0, shadowBuffer.depthAttachment->GetDefaultView(), *defaultSampler);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *scenePipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 1, 1, &*shadowMatrixDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 3, 1, &*sceneShadowTexDescriptor, 0, nullptr);

	DrawObjects(scenePipeline);

	SubmitDrawCall(floorObject.mesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}

void ShadowMappingExample::RenderFrame()	{
	RenderShadowMap();	
	RenderScene();
}

void ShadowMappingExample::DrawObjects(VulkanPipeline& toPipeline) {
	defaultCmdBuffer.pushConstants(toPipeline.layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&boxObject.transform);
	SubmitDrawCall(boxObject.mesh, defaultCmdBuffer);

	defaultCmdBuffer.pushConstants(toPipeline.layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&floorObject.transform);
	SubmitDrawCall(floorObject.mesh, defaultCmdBuffer);
}