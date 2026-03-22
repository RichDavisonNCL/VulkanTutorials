/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "MultiTexturingExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(MultiTexturingExample)

MultiTexturingExample::MultiTexturingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();
	FrameContext const& context = m_renderer->GetFrameContext();

	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	textures[2] = LoadTexture("Vulkan.png");
	textures[3] = LoadTexture("Doge.png");
	
	mesh = GenerateTriangle();
	
	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicTexturing.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("MultiTexturing.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
	.Build("Texturing Pipeline");

	descriptorSets[0] = CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(0));
	WriteCombinedImageDescriptor(context.device, *descriptorSets[0], 0, textures[0]->GetDefaultView(), *m_defaultSampler);
	WriteCombinedImageDescriptor(context.device, *descriptorSets[0], 1, textures[1]->GetDefaultView(), *m_defaultSampler);

	descriptorSets[1] = CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(1));
	WriteCombinedImageDescriptor(context.device, *descriptorSets[1], 0, 0, textures[0]->GetDefaultView(), *m_defaultSampler);
	WriteCombinedImageDescriptor(context.device, *descriptorSets[1], 0, 1, textures[1]->GetDefaultView(), *m_defaultSampler);
}

void MultiTexturingExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorSet	sets[2] = {
		*descriptorSets[0],
		*descriptorSets[1]
	};

	context.cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, //Which type of pipeline to fill
		*pipeline.layout,				//Which layout describes our pipeline
		0,								//First set to fill
		2,								//How many sets to fill
		sets,							//Pointer to our sets
		0,								//Dynamic offset count
		nullptr							//Pointer to dynamic offsets
	);

	mesh->Draw(context.cmdBuffer);
}