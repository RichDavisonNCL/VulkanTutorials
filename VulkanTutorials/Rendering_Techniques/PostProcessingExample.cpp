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

//TUTORIAL_ENTRY(PostProcessingExample)

//PostProcessingExample::PostProcessingExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
//	autoBeginDynamicRendering = false;
//}
//
//void PostProcessingExample::SetupTutorial() {
//	VulkanTutorial::SetupTutorial();
//	quadMesh = GenerateQuad();
//	gridMesh = GenerateGrid();
//
//	postTexture = TextureBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
//		.UsingPool(renderer->GetCommandPool(CommandBuffer::Graphics))
//		.UsingQueue(renderer->GetQueue(CommandBuffer::Graphics))
//		.WithDimension(windowSize.x, windowSize.y, 1)
//		.WithMips(false)
//		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled /*| vk::ImageUsageFlagBits::eStorage*/)
//		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
//		.WithLayout(vk::ImageLayout::eColorAttachmentOptimal)
//		.WithFormat(vk::Format::eB8G8R8A8Unorm)
//	.Build("Post Process Image");
//
//	gridShader = ShaderBuilder(renderer->GetDevice())
//		.WithVertexBinary("SimpleVertexTransform.vert.spv")
//		.WithFragmentBinary("BasicUniformBuffer.frag.spv")
//	.Build("Grid Shader");
//
//	invertShader = ShaderBuilder(renderer->GetDevice())
//		.WithVertexBinary("PostProcess.vert.spv")
//		.WithFragmentBinary("PostProcess.frag.spv")
//	.Build("Post process shader!");
//
//	BuildMainPipeline();
//	BuildProcessPipeline();
//
//	gridObject.mesh			= &*gridMesh;
//	gridObject.transform	= Matrix::Translation(Vector3(0, 0, -50)) * Matrix::Scale(Vector3(100, 100, 100));
//}
//
//void	PostProcessingExample::BuildMainPipeline() {		
//
//	gridPipeline = PipelineBuilder(renderer->GetDevice())
//		.WithVertexInputState(gridMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleList)
//		.WithShader(gridShader)
//	.Build("Main Scene Pipeline");
//
//	gridObject.descriptorSet = CreateDescriptorSet(gridShader->GetLayout(0));
//	WriteBufferDescriptor(renderer->GetDevice(), *gridObject.descriptorSet, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
//}
//
//void	PostProcessingExample::BuildProcessPipeline() {
//	invertPipeline = PipelineBuilder(renderer->GetDevice())
//		.WithVertexInputState(quadMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
//		.WithShader(invertShader)
//		.WithColourAttachment(renderer->GetSurfaceFormat())
//		.WithDepthAttachment(renderer->GetDepthBuffer()->GetDepthBuffer()->GetFormat())
//	.Build("Post process pipeline");
//
//	processDescriptor = CreateDescriptorSet(renderer->GetDevice(), invertShader->GetLayout(0));
//
//	WriteImageDescriptor(renderer->GetDevice(), *processDescriptor, 0, 0, postTexture->GetDefaultView(), *defaultSampler);
//
//	processSampler = renderer->GetDevice().createSamplerUnique(
//		vk::SamplerCreateInfo()
//		.setAnisotropyEnable(false)
//		.setMaxAnisotropy(1)
//	);
//}
//
//void PostProcessingExample::RenderFrame() {
//	DynamicRenderBuilder()
//		.WithColourAttachment(postTexture->GetDefaultView())
//		.WithRenderArea(defaultScreenRect)
//	.BeginRendering(frameCmds);
//
//	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, gridPipeline);
//	RenderSingleObject(gridObject, frameCmds, gridPipeline);
//	frameCmds.endRendering();
//
//	//Invert post process pass!
//	TransitionColourToSampler(frameCmds, *postTexture); //Sample first pass texture
//	BeginDefaultRendering(frameCmds);
//
//	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, invertPipeline);
//	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *invertPipeline.layout, 0, 1, &*processDescriptor, 0, nullptr);
//	quadMesh->Draw(frameCmds);
//
//	frameCmds.endRendering();
//
//	//Get read for the next frame's rendering!
//	TransitionSamplerToColour(frameCmds, *postTexture);
//}