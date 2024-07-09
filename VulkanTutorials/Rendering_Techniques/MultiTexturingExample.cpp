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

MultiTexturingExample::MultiTexturingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device		= renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	
	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	textures[2] = LoadTexture("Vulkan.png");
	textures[3] = LoadTexture("Doge.png");
	
	mesh = GenerateTriangle();
	
	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("MultiTexturing.frag.spv")
	.Build("Texturing Shader!");

	FrameState const& frameState = renderer->GetFrameState();
	
	pipeline = PipelineBuilder(device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
	.Build("Texturing Pipeline");

	descriptorSets[0] = CreateDescriptorSet(device, pool, shader->GetLayout(0));
	WriteImageDescriptor(device, *descriptorSets[0], 0, textures[0]->GetDefaultView(), *defaultSampler);
	WriteImageDescriptor(device, *descriptorSets[0], 1, textures[1]->GetDefaultView(), *defaultSampler);

	descriptorSets[1] = CreateDescriptorSet(device, pool, shader->GetLayout(1));
	WriteImageDescriptor(device, *descriptorSets[1], 0, 0, textures[0]->GetDefaultView(), *defaultSampler);
	WriteImageDescriptor(device, *descriptorSets[1], 0, 1, textures[1]->GetDefaultView(), *defaultSampler);
}

void MultiTexturingExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorSet	sets[2] = {
		*descriptorSets[0],
		*descriptorSets[1]
	};

	state.cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, //Which type of pipeline to fill
		*pipeline.layout,				//Which layout describes our pipeline
		0,								//First set to fill
		2,								//How many sets to fill
		sets,							//Pointer to our sets
		0,								//Dynamic offset count
		nullptr							//Pointer to dynamic offsets
	);

	mesh->Draw(state.cmdBuffer);
}