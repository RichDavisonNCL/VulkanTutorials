/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicTexturingRenderer.h"
#include "../../Plugins/VulkanRendering/Vulkan.h"

using namespace NCL;
using namespace Rendering;

BasicTexturingRenderer::BasicTexturingRenderer(Window& window) : VulkanTutorialRenderer(window)	{
	textures[0] = VulkanTexture::TexturePtrFromFilename("Vulkan.png");
	textures[1] = VulkanTexture::TexturePtrFromFilename("Doge.png");

	defaultMesh = std::shared_ptr<VulkanMesh>((VulkanMesh*)MeshGeometry::GenerateTriangle(new VulkanMesh()));
	defaultMesh->UploadToGPU(this);

	defaultShader = VulkanShaderBuilder()
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
		.WithDebugName("Texturing Shader!")
	.BuildUnique(device);

	BuildPipeline();
}

BasicTexturingRenderer::~BasicTexturingRenderer()	{

}

void	BasicTexturingRenderer::BuildPipeline() {
	descriptorLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Texture Layout A")
	.BuildUnique(device);

	texturePipeline = VulkanPipelineBuilder()
		.WithVertexSpecification(defaultMesh->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(&*defaultShader)
		.WithDescriptorSetLayout(*descriptorLayout) //Two separate descriptor sets!
		.WithDescriptorSetLayout(*descriptorLayout)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDebugName("TexturingPipeline")
	.Build(device, pipelineCache);

	for (int i = 0; i < 2; ++i) {
		descriptorSets[i] = BuildUniqueDescriptorSet(descriptorLayout.get());
		UpdateImageDescriptor(*descriptorSets[i], 0, textures[i]->GetDefaultView(), *defaultSampler);
	}
}

void BasicTexturingRenderer::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, texturePipeline.GetPipeline());

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, texturePipeline.GetLayout(), 0, 1, &*descriptorSets[0], 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, texturePipeline.GetLayout(), 1, 1, &*descriptorSets[1], 0, nullptr);

	SubmitDrawCall(defaultMesh.get(), defaultCmdBuffer);

	EndRendering(defaultCmdBuffer);
}