///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
#include "TextureUploadExample.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(TextureUploadExample)

const uint32_t dataWidth  = 128;
const uint32_t dataHeight = 128;

TextureUploadExample::TextureUploadExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	m_vkInit.autoBeginDynamicRendering = false;

	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	texture = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(context.cmdBuffer)
		.WithFormat(vk::Format::eR32G32B32A32Sfloat) //4 channels, 32bit float each
		.WithDimension(dataWidth, dataHeight)
		.Build("Custom Data");

	stagingBuffer = m_memoryManager->CreateStagingBuffer(dataWidth * dataHeight * 4 * sizeof(float), "Staging Buffer");
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
	WriteCombinedImageDescriptor(context.device, *descriptorSet, 0, texture->GetDefaultView(), *m_defaultSampler);
}

void TextureUploadExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	if (Window::GetKeyboard()->KeyDown(KeyCodes::T)) {
		UploadTexture(context.cmdBuffer);
	}

	m_renderer->BeginRenderToScreen(context.cmdBuffer);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	context.cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, //Which type of pipeline to fill
		*pipeline.layout,				//Which layout describes our pipeline
		0,								//First set to fill
		1,								//How many sets to fill
		&*descriptorSet,							//Pointer to our sets
		0,								//Dynamic offset count
		nullptr							//Pointer to dynamic offsets
	);

	mesh->Draw(context.cmdBuffer);

	context.cmdBuffer.endRendering();
}

void TextureUploadExample::UploadTexture(vk::CommandBuffer m_cmdBuffer) {
	Vector4* texData = (Vector4*)stagingBuffer.Map();

	for (int y = 0; y < dataHeight; ++y) {
		for (int x = 0; x < dataWidth; ++x) {
			Vector4* texel = &texData[(y * dataWidth) + x];
			*texel = Vector4(
				std::sin(m_runTime + (x / ((float)dataWidth - 1))),
				std::cos(m_runTime + (y / ((float)dataHeight - 1))), 
				0.0f, 
				1.0f);
		}
	}
	stagingBuffer.Unmap();

	Vulkan::UploadTextureData(m_cmdBuffer, stagingBuffer.buffer, texture->GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
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