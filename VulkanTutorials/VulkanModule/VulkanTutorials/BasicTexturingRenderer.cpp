/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicTexturingRenderer.h"
#include "../../Plugins/VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

BasicTexturingRenderer::BasicTexturingRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	textures[0] = VulkanTexture::TextureFromFile("Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile("Doge.png");

	defaultMesh = GenerateTriangle();

	defaultShader = VulkanShaderBuilder("Texturing Shader!")
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
	.Build(device);

	BuildPipeline();
}

void	BasicTexturingRenderer::BuildPipeline() {
	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Texture Layout A")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
	.Build(device);

	texturePipeline = VulkanPipelineBuilder("Texturing Pipeline")
		.WithVertexInputState(defaultMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(defaultShader)
		.WithDescriptorSetLayout(*descriptorLayout) //Two separate descriptor sets!
		.WithDescriptorSetLayout(*descriptorLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthFormat(depthBuffer->GetFormat())
	.Build(device, pipelineCache);

	for (int i = 0; i < 2; ++i) {
		descriptorSets[i] = BuildUniqueDescriptorSet(*descriptorLayout);
		UpdateImageDescriptor(*descriptorSets[i], 0, 0, textures[i]->GetDefaultView(), *defaultSampler);
	}
}

void BasicTexturingRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithRenderArea(defaultScreenRect)
	.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *texturePipeline.pipeline);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 0, 1, &*descriptorSets[0], 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline.layout, 1, 1, &*descriptorSets[1], 0, nullptr);

	SubmitDrawCall(*defaultMesh, defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}