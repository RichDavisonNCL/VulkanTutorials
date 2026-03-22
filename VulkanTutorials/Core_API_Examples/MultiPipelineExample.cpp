///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
//#include "MultiPipelineExample.h"
//
//using namespace NCL;
//using namespace Rendering;
//using namespace Vulkan;
//
//TUTORIAL_ENTRY(MultiPipelineExample)
//
//MultiPipelineExample::MultiPipelineExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
//	m_renderer = new VulkanRenderer(window, m_vkInit);
//	Initialise();
//
//	textures[0] = LoadTexture("Vulkan.png");
//	textures[1] = LoadTexture("doge.png");
//
//	triangleMesh = GenerateTriangle();
//	triangleMesh = GenerateQuad();
//
//	solidColourShader = ShaderBuilder(m_renderer->GetDevice())
//		.WithVertexBinary("BasicPushConstant.vert.spv")
//		.WithFragmentBinary("SolidColour.frag.spv")
//	.Build("Solid Colour Shader");
//
//	texturingShader = ShaderBuilder(m_renderer->GetDevice())
//		.WithVertexBinary("BasicPushConstant.vert.spv")
//		.WithFragmentBinary("SingleTexture.frag.spv")
//	.Build("Texturing Shader");
//
//	FrameContext const& context = m_renderer->GetFrameContext();
//	vk::Device device = m_renderer->GetDevice();
//	vk::DescriptorPool m_pool = m_renderer->GetDescriptorPool();
//
//	solidColourPipeline = PipelineBuilder(m_renderer->GetDevice())
//		.WithVertexInputState(triangleMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleList)
//		.WithColourAttachment(context.colourFormat, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
//		.WithDepthAttachment(context.depthFormat)
//		.WithShader(solidColourShader)
//	.Build("Solid colour Pipeline!");
//
//	texturedPipeline = PipelineBuilder(m_device)
//		.WithVertexInputState(triangleMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleList)
//		.WithColourAttachment(context.colourFormat, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
//		.WithDepthAttachment(context.depthFormat)
//		.WithShader(texturingShader)
//		.Build("Texturing Pipeline");
//
//	descriptorSets[0] = CreateDescriptorSet(m_device, m_pool, texturingShader->GetSetLayout(1));
//	descriptorSets[1] = CreateDescriptorSet(m_device, m_pool, texturingShader->GetSetLayout(1));
//	WriteCombinedImageDescriptor(m_device, *descriptorSets[0], 0, textures[0]->GetDefaultView(), *m_defaultSampler);
//	WriteCombinedImageDescriptor(m_device, *descriptorSets[1], 0, textures[1]->GetDefaultView(), *m_defaultSampler);
//
//}
//
//void MultiPipelineExample::RenderFrame(float dt) {
//	Vector4 objectPositions[3] = { Vector4(0.5, 0, 0, 1), Vector4(-0.5, 0, 0, 1), Vector4(0, 0.5, 0, 1) };
//
//	FrameContext const& context = m_renderer->GetFrameContext();
//
//	//First draw the solid colour triangle
//	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, solidColourPipeline);
//	context.cmdBuffer.pushConstants(*solidColourPipeline.m_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[0]);
//
//	triangleMesh->Draw(context.cmdBuffer);
//
//	////Now to draw the two textured triangles!
//	////They both use the same pipeline, but different descriptors, as they have different textures
//	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, texturedPipeline);
//	context.cmdBuffer.pushConstants(*texturedPipeline.m_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[1]);
//	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.m_layout, 1, 1, &*descriptorSets[0], 0, nullptr);
//	triangleMesh->Draw(context.cmdBuffer);
//
//	context.cmdBuffer.pushConstants(*texturedPipeline.m_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vector4), (void*)&objectPositions[2]);
//	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturedPipeline.m_layout, 1, 1, &*descriptorSets[1], 0, nullptr);
//	triangleMesh->Draw(context.cmdBuffer);
//}