/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PostProcessingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

PostProcessingExample::PostProcessingExample(Window& window) : VulkanTutorialRenderer(window) {
	autoBeginDynamicRendering = false;
}

void PostProcessingExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	quadMesh = GenerateQuad();
	gridMesh = GenerateGrid();

	//postTexture = VulkanTexture::CreateColourTexture(this, windowSize.x, windowSize.y, "Post Process Image");

	postTexture = TextureBuilder(GetDevice(), GetMemoryAllocator())
		.WithPool(GetCommandPool(CommandBuffer::Graphics))
		.WithQueue(GetQueue(CommandBuffer::Graphics))
		.WithDimension(windowSize.x, windowSize.y, 1)
		.WithMips(false)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled /*| vk::ImageUsageFlagBits::eStorage*/)
		.WithPipeFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.WithFormat(vk::Format::eB8G8R8A8Unorm)
	.Build("Post Process Image");


	gridShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build("Grid Shader");

	invertShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("PostProcess.vert.spv")
		.WithFragmentBinary("PostProcess.frag.spv")
	.Build("Post process shader!");

	BuildMainPipeline();
	BuildProcessPipeline();

	gridObject.mesh			= &*gridMesh;
	gridObject.transform	= Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(100, 100, 100));
}

void	PostProcessingExample::BuildMainPipeline() {		
	matrixLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build("Matrices");

	gridPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(gridMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gridShader)
		.WithDescriptorSetLayout(0, *matrixLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex,0, sizeof(Matrix4))
	.Build("Main Scene Pipeline");

	gridObject.descriptorSet = BuildUniqueDescriptorSet(*matrixLayout);
	WriteBufferDescriptor(*gridObject.descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
}

void	PostProcessingExample::BuildProcessPipeline() {
	processLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Texture Layout");

	invertPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(invertShader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *processLayout)
	.Build("Post process pipeline");

	processDescriptor = BuildUniqueDescriptorSet(*processLayout);

	WriteImageDescriptor(*processDescriptor, 0, 0, postTexture->GetDefaultView(), *defaultSampler);

	processSampler = GetDevice().createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(1)
	);
}

void PostProcessingExample::RenderFrame() {
	DynamicRenderBuilder()
		.WithColourAttachment(postTexture->GetDefaultView())
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, gridPipeline);
	RenderSingleObject(gridObject, frameCmds, gridPipeline);
	frameCmds.endRendering();

	//Invert post process pass!
	TransitionColourToSampler(frameCmds, *postTexture); //Sample first pass texture
	BeginDefaultRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, invertPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *invertPipeline.layout, 0, 1, &*processDescriptor, 0, nullptr);
	DrawMesh(frameCmds, *quadMesh);

	frameCmds.endRendering();

	//Get read for the next frame's rendering!
	TransitionSamplerToColour(frameCmds, *postTexture);
}