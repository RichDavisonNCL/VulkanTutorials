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
}

void BasicMultiPipelineRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	textures[0] = VulkanTexture::TextureFromFile(this, "Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile(this, "doge.png");

	triangleMesh = GenerateTriangle();
	triangleMesh = GenerateQuad();

	solidColourShader = VulkanShaderBuilder("Solid Colour Shader")
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SolidColour.frag.spv")
	.Build(GetDevice());

	texturingShader = VulkanShaderBuilder("Texturing Shader")
		.WithVertexBinary("BasicPushConstant.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build(GetDevice());

	BuildColourPipeline();
	BuildTexturePipeline();
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
	.Build(GetDevice());
}

void BasicMultiPipelineRenderer::BuildTexturePipeline() {
	vk::UniqueDescriptorSetLayout textureLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	texturedPipeline = VulkanPipelineBuilder("Texturing Pipeline")
		.WithVertexInputState(triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(texturingShader)
		.WithDescriptorSetLayout(1, *textureLayout)
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4))
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, true)
		.WithDepthFormat(depthBuffer->GetFormat())
	.Build(GetDevice());

	textureDescriptorSetA = BuildUniqueDescriptorSet(*textureLayout);
	textureDescriptorSetB = BuildUniqueDescriptorSet(*textureLayout);
	UpdateImageDescriptor(*textureDescriptorSetA, 0, 0, textures[0]->GetDefaultView(), *defaultSampler);
	UpdateImageDescriptor(*textureDescriptorSetB, 0, 0, textures[1]->GetDefaultView(), *defaultSampler);
}

void BasicMultiPipelineRenderer::RenderFrame() {
	Vector4 objectPositions[3] = { Vector4(0.5, 0, 0, 1), Vector4(-0.5, 0, 0, 1), Vector4(0, 0.5, 0, 1) };

	//First draw the solid colour triangle
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, solidColourPipeline);
	frameCmds.pushConstants(*solidColourPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[0]);

	SubmitDrawCall(frameCmds, *triangleMesh);

	////Now to draw the two textured triangles!
	////They both use the same pipeline, but different descriptors, as they have different textures
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, texturedPipeline);
	frameCmds.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[1]);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &*textureDescriptorSetA, 0, nullptr);
	SubmitDrawCall(frameCmds, *triangleMesh);

	frameCmds.pushConstants(*texturedPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[2]);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.layout, 1, 1, &*textureDescriptorSetB, 0, nullptr);
	SubmitDrawCall(frameCmds, *triangleMesh);
}