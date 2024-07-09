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

const uint32_t dataWidth  = 128;
const uint32_t dataHeight = 128;

TextureUploadExample::TextureUploadExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	vkInit.autoBeginDynamicRendering = false;

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	vk::Device device			 = renderer->GetDevice();
	
	texture = TextureBuilder(device, renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandType::Graphics))
		.UsingQueue(renderer->GetQueue(CommandType::Graphics))
		.WithFormat(vk::Format::eR32G32B32A32Sfloat) //4 channels, 32bit float each
		.WithDimension(dataWidth, dataHeight)
		.Build("Custom Data");

	stagingBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.WithHostVisibility()
		.Build(dataWidth * dataHeight * 4 * sizeof(float), "Staging Buffer");
	
	mesh = GenerateQuad();
	
	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicTexturing.vert.spv")
		.WithFragmentBinary("BasicTexturing.frag.spv")
	.Build("Texturing Shader!");

	FrameState const& frameState = renderer->GetFrameState();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(mesh->GetVulkanTopology())
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
	.Build("Texturing Pipeline");

	descriptorSet = CreateDescriptorSet(device, pool, shader->GetLayout(0));
	WriteImageDescriptor(device, *descriptorSet, 0, texture->GetDefaultView(), *defaultSampler);
}

void TextureUploadExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();

	if (Window::GetKeyboard()->KeyDown(KeyCodes::T)) {
		UploadTexture(state.cmdBuffer);
	}

	renderer->BeginDefaultRendering(state.cmdBuffer);

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

	state.cmdBuffer.endRendering();
}

void TextureUploadExample::UploadTexture(vk::CommandBuffer cmdBuffer) {
	Vector4* texData = (Vector4*)stagingBuffer.Map();

	for (int y = 0; y < dataHeight; ++y) {
		for (int x = 0; x < dataWidth; ++x) {
			Vector4* texel = &texData[(y * dataWidth) + x];
			*texel = Vector4(
				std::sin(runTime + (x / ((float)dataWidth - 1))),
				std::cos(runTime + (y / ((float)dataHeight - 1))), 
				0.0f, 
				1.0f);
		}
	}
	stagingBuffer.Unmap();

	Vulkan::UploadTextureData(cmdBuffer, stagingBuffer.buffer, texture->GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::BufferImageCopy{
			.imageSubresource = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel	= 0,
				.layerCount = 1
			},
			.imageExtent{dataWidth, dataHeight, 1},
		}
	);
}