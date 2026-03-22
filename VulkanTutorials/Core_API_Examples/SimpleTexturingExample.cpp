///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
#include "SimpleTexturingExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(SimpleTexturingExample)

SimpleTexturingExample::SimpleTexturingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();
	
	texture = LoadTexture("Vulkan.png");
	
	mesh = GenerateQuad();
	
	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(mesh->GetVulkanTopology())
		.WithShaderBinary("BasicTexturing.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicTexturing.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
	.Build("Texturing Pipeline");

	descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(0));
//WriteCombinedImageDescriptor(context.device, *descriptorSet, 0, texture->GetDefaultView(), *m_defaultSampler);
	WriteDescriptor(context.device,
	{
		.dstSet				= *descriptorSet,
		.dstBinding			= 0,
		.descriptorCount	= 1,
		.descriptorType		= vk::DescriptorType::eCombinedImageSampler
	}
	,
	{
		.sampler		= *m_defaultSampler,
		.imageView		= texture->GetDefaultView(),
		.imageLayout	= vk::ImageLayout::eShaderReadOnlyOptimal
	});
}

void SimpleTexturingExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	context.cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,	//Which type of pipeline to fill
		*pipeline.layout,					//Which layout describes our pipeline
		0,									//First set to fill
		1,									//How many sets to fill
		&*descriptorSet,					//Pointer to our sets
		0,									//Dynamic offset count
		nullptr								//Pointer to dynamic offsets
	);

	mesh->Draw(context.cmdBuffer);
}