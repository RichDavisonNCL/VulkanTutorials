/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "PostProcessingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(PostProcessingExample)

PostProcessingExample::PostProcessingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.autoBeginDynamicRendering = false;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	postTexture = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(context.cmdBuffer)
		.WithDimension(context.viewport.width, std::abs(context.viewport.height), 1)
		.WithMips(false)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.WithFormat(vk::Format::eB8G8R8A8Unorm)
	.Build("Post Process Image");

	gridPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_gridMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("SimpleVertexTransform.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BasicUniformBuffer.frag.spv", vk::ShaderStageFlagBits::eFragment)
	.Build("Main Scene Pipeline");

	gridObject.descriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, gridPipeline.GetSetLayout(0));
	WriteBufferDescriptor(context.device, *gridObject.descriptorSet, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);

	invertPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShaderBinary("PostProcess.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("PostProcess.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
	.Build("Post process pipeline");

	processDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, invertPipeline.GetSetLayout(0));

	WriteCombinedImageDescriptor(context.device, *processDescriptor, 0, 0, postTexture->GetDefaultView(), *m_defaultSampler);

	processSampler = context.device.createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(1)
	);

	gridObject.mesh			= &*m_gridMesh;
	gridObject.transform	= Matrix::Translation(Vector3(0, 0, -50)) * Matrix::Scale(Vector3(100, 100, 100));
}

void PostProcessingExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	DynamicRenderBuilder()
		.WithColourAttachment(postTexture->GetDefaultView())
		.WithRenderArea(context.screenRect)
	.BeginRendering(context.cmdBuffer);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gridPipeline);
	RenderSingleObject(gridObject, context.cmdBuffer, gridPipeline);
	context.cmdBuffer.endRendering();

	//Invert post process pass!
	TransitionColourToSampler(context.cmdBuffer, *postTexture); //Sample first pass texture
	m_renderer->BeginRenderToScreen(context.cmdBuffer);

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, invertPipeline);
	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *invertPipeline.layout, 0, 1, &*processDescriptor, 0, nullptr);
	m_quadMesh->Draw(context.cmdBuffer);

	context.cmdBuffer.endRendering();

	//Get read for the next frame's rendering!
	TransitionSamplerToColour(context.cmdBuffer, *postTexture);
}