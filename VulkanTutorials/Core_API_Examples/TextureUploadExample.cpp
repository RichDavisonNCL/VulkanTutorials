/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "TextureUploadExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(TextureUploadExample)

TextureUploadExample::TextureUploadExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device		= renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();
	
	textures[0] = LoadTexture("Vulkan.png");

	const uint32_t dataWidth = 128;
	const uint32_t dataHeight = 128;

	Vector4* data = new Vector4[dataWidth * dataHeight];

	for (int y = 0; y < dataHeight; ++y) {
		for (int x = 0; x < dataWidth; ++x) {
			Vector4* texel = &data[(y * dataWidth) + x];
			*texel = Vector4(x / ((float)dataWidth - 1), y / ((float)dataHeight - 1), 0.0f, 1.0f);
		}
	} 

	textures[1] = TextureBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandType::Graphics))
		.UsingQueue(renderer->GetQueue(CommandType::Graphics))
		.WithFormat(vk::Format::eR32G32B32A32Sfloat) //4 channels, 32bit float each
		.WithDimension(dataWidth, dataHeight)
		.BuildFromData(data, dataWidth * dataHeight * sizeof(Vector4), "Custom Data");

	delete data;
	
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
	
	for (int i = 0; i < 2; ++i) {
		descriptorSets[i] = CreateDescriptorSet(device, pool, shader->GetLayout(i));
		WriteImageDescriptor(device , *descriptorSets[i], 0, textures[i]->GetDefaultView(), *defaultSampler);
	}
}

void TextureUploadExample::RenderFrame(float dt) {
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