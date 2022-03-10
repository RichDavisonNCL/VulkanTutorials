/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "PostProcessingExample.h"

using namespace NCL;
using namespace Rendering;

PostProcessingExample::PostProcessingExample(Window& window) : VulkanTutorialRenderer(window) {
	quadMesh = MakeSmartMesh(GenerateQuad());
	gridMesh = MakeSmartMesh(GenerateGrid());

	sceneBuffer = BuildFrameBuffer(defaultRenderPass, windowWidth, windowHeight, 1, true);

	LoadShaders();
	BuildMainPipeline();
	BuildProcessPipeline();

	gridObject.mesh			= gridMesh.get(); //HMM
	gridObject.transform	= Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(100, 100, 100));
}

PostProcessingExample::~PostProcessingExample() {
}

void	PostProcessingExample::BuildMainPipeline() {
	vk::PushConstantRange modelMatrixConstant = vk::PushConstantRange()
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setOffset(0)
		.setSize(sizeof(Matrix4));
		
	matrixLayout = VulkanDescriptorSetLayoutBuilder("Matrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.BuildUnique(device);

	gridPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexSpecification(gridMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(gridShader.get())
		.WithPass(defaultRenderPass)
		.WithDescriptorSetLayout(matrixLayout.get())
		.WithPushConstant(modelMatrixConstant)
	.Build(device, pipelineCache);

	gridObject.objectDescriptorSet = BuildUniqueDescriptorSet(matrixLayout.get());
	UpdateUniformBufferDescriptor(gridObject.objectDescriptorSet.get(), cameraUniform.cameraData, 0);
}

void	PostProcessingExample::BuildProcessPipeline() {
	processLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device);

	invertPipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexSpecification(quadMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShaderState(invertShader.get())
		.WithPass(defaultRenderPass)
		.WithDescriptorSetLayout(processLayout.get())
	.Build(device, pipelineCache);

	processDescriptor = BuildUniqueDescriptorSet(processLayout.get());

	processSampler = device.createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(1)
	);
}

void PostProcessingExample::LoadShaders() {
	gridShader = VulkanShaderBuilder("Grid Shader")
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.BuildUnique(device);

	invertShader = VulkanShaderBuilder("Post process shader!")
		.WithVertexBinary("PostProcess.vert.spv")
		.WithFragmentBinary("PostProcess.frag.spv")
	.BuildUnique(device);
}

void PostProcessingExample::RenderFrame() {
	ImageTransitionBarrier(defaultCmdBuffer, sceneBuffer.colourAttachments[0].get(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput);

	vk::ClearValue clearValues[] =
	{
		ClearColour(0.2f, 0.2f, 0.2f, 1.0f),
		vk::ClearDepthStencilValue(1.0f, 0),
	};

	vk::RenderPassBeginInfo passBegin = vk::RenderPassBeginInfo()//This first pass will render our scene into a framebuffer
		.setRenderPass(defaultRenderPass)
		.setFramebuffer(sceneBuffer.frameBuffer.get())
		.setRenderArea(defaultScissor)
		.setClearValueCount(sizeof(clearValues) / sizeof(vk::ClearValue))
		.setPClearValues(clearValues);

	defaultCmdBuffer.beginRenderPass(passBegin, vk::SubpassContents::eInline);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gridPipeline.pipeline.get());
	RenderSingleObject(gridObject, defaultCmdBuffer, gridPipeline);
	defaultCmdBuffer.endRenderPass();
//This bit will then perform post processing on our scene
	ImageTransitionBarrier(defaultCmdBuffer, sceneBuffer.colourAttachments[0].get(),
	vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor, 
	vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

	UpdateImageDescriptor(processDescriptor.get(), 0, sceneBuffer.colourAttachments[0]->GetDefaultView(), processSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::RenderPassBeginInfo secondPass = passBegin; //we'll be borrowing a lot of the settings from the first pass!
	secondPass.setFramebuffer(frameBuffers[currentSwap]);//EXCEPT we're going to render to the back buffer, not our separate FB!

	defaultCmdBuffer.beginRenderPass(secondPass, vk::SubpassContents::eInline);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, invertPipeline.pipeline.get());
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, invertPipeline.layout.get(), 0, 1, &processDescriptor.get(), 0, nullptr);
	SubmitDrawCall(quadMesh.get(), defaultCmdBuffer);

	defaultCmdBuffer.endRenderPass();
}