/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicDescriptorRenderer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

BasicDescriptorRenderer::BasicDescriptorRenderer(Window& window) : VulkanTutorialRenderer(window)	{	
}

void BasicDescriptorRenderer::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	triMesh = GenerateTriangle();

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicDescriptor.vert.spv")
		.WithFragmentBinary("BasicDescriptor.frag.spv")
	.Build("Basic Descriptor Shader!");
	
	descriptorLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eFragment)
	.Build("Uniform Data");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment( GetSurfaceFormat() )
		.WithDepthAttachment(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *descriptorLayout)
	.Build("Basic Descriptor pipeline");

	descriptorSet = BuildUniqueDescriptorSet(*descriptorLayout);

	uniformData[0] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Positions");

	uniformData[1] = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(positionUniform), "Colours");

	WriteBufferDescriptor(*descriptorSet, 0, vk::DescriptorType::eUniformBuffer, uniformData[0]);
	WriteBufferDescriptor(*descriptorSet, 1, vk::DescriptorType::eUniformBuffer, uniformData[1]);
}

void BasicDescriptorRenderer::RenderFrame() {
	Vector3 positionUniform = Vector3(sin(runTime), 0.0, 0.0f);
	Vector4 colourUniform	= Vector4(sin(runTime), 0, 1, 1);

	uniformData[0].CopyData((void*)&positionUniform, sizeof(positionUniform));
	uniformData[1].CopyData((void*)&colourUniform, sizeof(colourUniform));

	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*descriptorSet, 0, nullptr);

	DrawMesh(frameCmds, *triMesh);
}