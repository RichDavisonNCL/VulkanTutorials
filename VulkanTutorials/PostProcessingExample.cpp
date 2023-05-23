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
	autoBeginDynamicRendering = false;
}

void PostProcessingExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	quadMesh = GenerateQuad();
	gridMesh = GenerateGrid();

	postTexture = VulkanTexture::CreateColourTexture(this, windowWidth, windowHeight, "Post Process Image");

	gridShader = VulkanShaderBuilder("Grid Shader")
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
	.Build(GetDevice());

	invertShader = VulkanShaderBuilder("Post process shader!")
		.WithVertexBinary("PostProcess.vert.spv")
		.WithFragmentBinary("PostProcess.frag.spv")
	.Build(GetDevice());

	BuildMainPipeline();
	BuildProcessPipeline();

	gridObject.mesh			= &*gridMesh;
	gridObject.transform	= Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(100, 100, 100));
}

void	PostProcessingExample::BuildMainPipeline() {		
	matrixLayout = VulkanDescriptorSetLayoutBuilder("Matrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
	.Build(GetDevice());

	gridPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithVertexInputState(gridMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(gridShader)
		.WithDescriptorSetLayout(0, *matrixLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex,0, sizeof(Matrix4))
	.Build(GetDevice());

	gridObject.objectDescriptorSet = BuildUniqueDescriptorSet(*matrixLayout);
	UpdateBufferDescriptor(*gridObject.objectDescriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
}

void	PostProcessingExample::BuildProcessPipeline() {
	processLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	invertPipeline = VulkanPipelineBuilder("Post process pipeline")
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(invertShader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *processLayout)
	.Build(GetDevice());

	processDescriptor = BuildUniqueDescriptorSet(*processLayout);

	UpdateImageDescriptor(*processDescriptor, 0, 0, postTexture->GetDefaultView(), *defaultSampler);

	processSampler = GetDevice().createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(1)
	);
}

void PostProcessingExample::RenderFrame() {
	VulkanDynamicRenderBuilder()
		.WithColourAttachment(postTexture->GetDefaultView())
		.WithRenderArea(defaultScreenRect)
	.BeginRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, gridPipeline);
	RenderSingleObject(gridObject, frameCmds, gridPipeline);
	EndRendering(frameCmds);

	//Invert post process pass!
	Vulkan::TransitionColourToSampler(frameCmds, *postTexture); //Sample first pass texture
	BeginDefaultRendering(frameCmds);

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, invertPipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *invertPipeline.layout, 0, 1, &*processDescriptor, 0, nullptr);
	SubmitDrawCall(frameCmds, *quadMesh);

	EndRendering(frameCmds);

	//Get read for the next frame's rendering!
	Vulkan::TransitionSamplerToColour(frameCmds, *postTexture);
}