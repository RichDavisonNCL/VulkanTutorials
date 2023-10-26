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
using namespace Vulkan;

BasicTexturingRenderer::BasicTexturingRenderer(Window& window) : VulkanTutorialRenderer(window)	{
}

void BasicTexturingRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	defaultMesh = GenerateTriangle();

	defaultShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
	.Build("Texturing Shader!");

	descriptorLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Texture Layout A");

	texturePipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(defaultMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(defaultShader)
		.WithDescriptorSetLayout(0,*descriptorLayout) //Two separate descriptor sets!
		.WithDescriptorSetLayout(1,*descriptorLayout)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
	.Build("Texturing Pipeline");

	for (int i = 0; i < 2; ++i) {
		descriptorSets[i] = BuildUniqueDescriptorSet(*descriptorLayout);
		WriteImageDescriptor(*descriptorSets[i], 0, 0, textures[i]->GetDefaultView(), *defaultSampler);
	}
}

void BasicTexturingRenderer::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, texturePipeline);

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 0, 1, &*descriptorSets[0], 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 1, 1, &*descriptorSets[1], 0, nullptr);

	DrawMesh(frameCmds, *defaultMesh);
}