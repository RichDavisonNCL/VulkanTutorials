/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicMultiPipelineRenderer.h"

using namespace NCL;
using namespace Rendering;

BasicMultiPipelineRenderer::BasicMultiPipelineRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	textures[0] = VulkanTexture::TextureFromFile("Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile("doge.png");

	triangleMesh = GenerateTriangle();
	triangleMesh = GenerateQuad();

	solidColourShader = VulkanShaderBuilder("Solid Colour Shader")
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SolidColour.frag.spv")
	.BuildUnique(device);

	texturingShader = VulkanShaderBuilder("Texturing Shader")
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.BuildUnique(device);

	BuildColourPipeline();
	BuildTexturePipeline();
}

BasicMultiPipelineRenderer::~BasicMultiPipelineRenderer()	{
}

void BasicMultiPipelineRenderer::BuildColourPipeline() {
	solidColourPipeline = VulkanPipelineBuilder("Solid colour Pipeline!")
		.WithVertexInputState(triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(solidColourShader)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex,0, sizeof(Vector4))
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
		.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
	.Build(device, pipelineCache);
}

void BasicMultiPipelineRenderer::BuildTexturePipeline() {
	vk::UniqueDescriptorSetLayout textureLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.BuildUnique(device);

	texturedPipeline = VulkanPipelineBuilder("Texturing Pipeline")
		.WithVertexInputState(triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(texturingShader)
		.WithDescriptorSetLayout(nullLayout)	//Nothing in slot 0
		.WithDescriptorSetLayout(textureLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4))
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
	.Build(device, pipelineCache);

	textureDescriptorSetA = BuildDescriptorSet(*textureLayout);
	textureDescriptorSetB = BuildDescriptorSet(*textureLayout);
	UpdateImageDescriptor(textureDescriptorSetA, 0, textures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(textureDescriptorSetB, 0, textures[1]->GetDefaultView(), *defaultSampler);
}

void BasicMultiPipelineRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	Vector4 objectPositions[3] = { Vector4(0.5, 0, 0, 1), Vector4(-0.5, 0, 0, 1), Vector4(0, 0.5, 0, 1) };

	//First draw the solid colour triangle
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *solidColourPipeline.pipeline);
	defaultCmdBuffer.pushConstants(*solidColourPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[0]);

	SubmitDrawCall(*triangleMesh, defaultCmdBuffer);

	////Now to draw the two textured triangles!
	////They both use the same pipeline, but different descriptors, as they have different textures
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *texturedPipeline.pipeline);
	defaultCmdBuffer.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[1]);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &textureDescriptorSetA, 0, nullptr);
	SubmitDrawCall(*triangleMesh, defaultCmdBuffer);

	defaultCmdBuffer.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[2]);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &textureDescriptorSetB, 0, nullptr);
	SubmitDrawCall(*triangleMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}