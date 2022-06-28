/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PostProcessingExample.h"

using namespace NCL;
using namespace Rendering;

PostProcessingExample::PostProcessingExample(Window& window) : VulkanTutorialRenderer(window) {
	quadMesh = GenerateQuad();
	gridMesh = GenerateGrid();

	postTexture = VulkanTexture::CreateColourTexture(windowWidth, windowHeight, "Post Process Image");

	gridShader = VulkanShaderBuilder("Grid Shader")
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build(device);

	invertShader = VulkanShaderBuilder("Post process shader!")
		.WithVertexBinary("PostProcess.vert.spv")
		.WithFragmentBinary("PostProcess.frag.spv")
	.Build(device);

	BuildMainPipeline();
	BuildProcessPipeline();

	gridObject.mesh			= &*gridMesh;
	gridObject.transform	= Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(100, 100, 100));
}

void	PostProcessingExample::BuildMainPipeline() {		
	matrixLayout = VulkanDescriptorSetLayoutBuilder("Matrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build(device);

	gridPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexInputState(gridMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gridShader)
		.WithDescriptorSetLayout(*matrixLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex,0, sizeof(Matrix4))
	.Build(device, pipelineCache);

	gridObject.objectDescriptorSet = BuildUniqueDescriptorSet(*matrixLayout);
	UpdateBufferDescriptor(*gridObject.objectDescriptorSet, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
}

void	PostProcessingExample::BuildProcessPipeline() {
	processLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device);

	invertPipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(invertShader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(*processLayout)
	.Build(device, pipelineCache);

	processDescriptor = BuildUniqueDescriptorSet(*processLayout);

	UpdateImageDescriptor(*processDescriptor, 0, 0, postTexture->GetDefaultView(), *defaultSampler);

	processSampler = device.createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(1)
	);
}

void PostProcessingExample::RenderFrame() {
	VulkanDynamicRenderBuilder()
		.WithColourAttachment(postTexture->GetDefaultView())
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *gridPipeline.pipeline);
	RenderSingleObject(gridObject, defaultCmdBuffer, gridPipeline);
	EndRendering(defaultCmdBuffer);

	//Invert post process pass!
	TransitionSwapchainForRendering(defaultCmdBuffer);
	TransitionColourToSampler(&*postTexture, defaultCmdBuffer); //Sample first pass texture
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *invertPipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *invertPipeline.layout, 0, 1, &*processDescriptor, 0, nullptr);
	SubmitDrawCall(*quadMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);

	//Get read for the next frame's rendering!
	TransitionSamplerToColour(&*postTexture, defaultCmdBuffer);
}