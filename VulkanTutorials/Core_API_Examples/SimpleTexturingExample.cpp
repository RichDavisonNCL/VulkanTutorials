/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "SimpleTexturingExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(SimpleTexturingExample)

SimpleTexturingExample::SimpleTexturingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device		= renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	
	texture = LoadTexture("Vulkan.png");
	
	mesh = GenerateQuad();
	
	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
	.Build("Texturing Shader!");

	FrameState const& frameState = renderer->GetFrameState();
	
	pipeline = PipelineBuilder(device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(mesh->GetVulkanTopology())
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
	.Build("Texturing Pipeline");

	descriptorSet = CreateDescriptorSet(device, pool, shader->GetLayout(0));
	WriteImageDescriptor(device , *descriptorSet, 0, texture->GetDefaultView(), *defaultSampler);
}

void SimpleTexturingExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	state.cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, //Which type of pipeline to fill
		*pipeline.layout,				//Which layout describes our pipeline
		0,								//First set to fill
		1,								//How many sets to fill
		&*descriptorSet,							//Pointer to our sets
		0,								//Dynamic offset count
		nullptr							//Pointer to dynamic offsets
	);

	mesh->Draw(state.cmdBuffer);
}