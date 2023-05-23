/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicTexturingRenderer.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

BasicTexturingRenderer::BasicTexturingRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

void BasicTexturingRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	textures[0] = VulkanTexture::TextureFromFile(this, "Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile(this, "Doge.png");

	defaultMesh = GenerateTriangle();

	defaultShader = VulkanShaderBuilder("Texturing Shader!")
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
	.Build(GetDevice());

	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout A")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(GetDevice());

	texturePipeline = VulkanPipelineBuilder("Texturing Pipeline")
		.WithVertexInputState(defaultMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(defaultShader)
		.WithDescriptorSetLayout(0,*descriptorLayout) //Two separate descriptor sets!
		.WithDescriptorSetLayout(1,*descriptorLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
	.Build(GetDevice());

	for (int i = 0; i < 2; ++i) {
		descriptorSets[i] = BuildUniqueDescriptorSet(*descriptorLayout);
		UpdateImageDescriptor(*descriptorSets[i], 0, 0, textures[i]->GetDefaultView(), *defaultSampler);
	}
}

void BasicTexturingRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, texturePipeline);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 0, 1, &*descriptorSets[0], 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 1, 1, &*descriptorSets[1], 0, nullptr);

	SubmitDrawCall(frameCmds, *defaultMesh);
}