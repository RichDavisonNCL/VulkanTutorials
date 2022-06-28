/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanPipelineBuilder.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

VulkanPipelineBuilder::VulkanPipelineBuilder(const std::string& pipeName)	{
	dynamicStateEnables[0] = vk::DynamicState::eViewport;
	dynamicStateEnables[1] = vk::DynamicState::eScissor;

	dynamicCreate.setDynamicStateCount(2);
	dynamicCreate.setPDynamicStates(dynamicStateEnables);

	sampleCreate.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	viewportCreate.setViewportCount(1);
	viewportCreate.setScissorCount(1);

	pipelineCreate.setPViewportState(&viewportCreate);

	depthStencilCreate.setDepthCompareOp(vk::CompareOp::eAlways)
		.setDepthTestEnable(false)
		.setDepthWriteEnable(false)
		.setStencilTestEnable(false)
		.setDepthBoundsTestEnable(false);

	depthRenderingFormat		= vk::Format::eUndefined;
	stencilRenderingFormat		= vk::Format::eUndefined;

	rasterCreate.setCullMode(vk::CullModeFlagBits::eNone)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setLineWidth(1.0f);

	debugName = pipeName;

	inputAsmCreate.setTopology(vk::PrimitiveTopology::eTriangleList);
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithDepthState(vk::CompareOp op, bool depthEnabled, bool writeEnabled, bool stencilEnabled) {
	depthStencilCreate.setDepthCompareOp(op)
		.setDepthTestEnable(depthEnabled)
		.setDepthWriteEnable(writeEnabled)
		.setStencilTestEnable(stencilEnabled);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithBlendState(vk::BlendFactor srcState, vk::BlendFactor dstState, bool isEnabled) {
	vk::PipelineColorBlendAttachmentState pipeBlend;

	pipeBlend.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(isEnabled)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorBlendOp(vk::BlendOp::eAdd)

		.setSrcAlphaBlendFactor(srcState)
		.setSrcColorBlendFactor(srcState)

		.setDstAlphaBlendFactor(dstState)
		.setDstColorBlendFactor(dstState);

	blendAttachStates.emplace_back(pipeBlend);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithRaster(vk::CullModeFlagBits cullMode, vk::PolygonMode polyMode) {
	rasterCreate.setCullMode(cullMode).setPolygonMode(polyMode);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithVertexInputState(const vk::PipelineVertexInputStateCreateInfo& spec) {
	vertexCreate = spec;
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithTopology(vk::PrimitiveTopology topology) {
	inputAsmCreate.setTopology(topology);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithShader(const UniqueVulkanShader& shader) {
	shader->FillShaderStageCreateInfo(pipelineCreate);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithLayout(vk::PipelineLayout layout) {
	this->layout = layout;
	pipelineCreate.setLayout(layout);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size) {
	allPushConstants.emplace_back(vk::PushConstantRange(flags, offset, size));
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithPass(vk::RenderPass& renderPass) {
	pipelineCreate.setRenderPass(renderPass);
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithDepthStencilFormat(vk::Format depthFormat) {
	depthRenderingFormat	= depthFormat;
	stencilRenderingFormat	= depthFormat;
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithDepthFormat(vk::Format depthFormat) {
	depthRenderingFormat = depthFormat;
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithColourFormats(const std::vector<vk::Format>& formats) {
	allColourRenderingFormats = formats;
	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::WithDescriptorSetLayout(vk::DescriptorSetLayout layout) {
	allLayouts.emplace_back(layout);
	return *this;
}

VulkanPipeline	VulkanPipelineBuilder::Build(vk::Device device, vk::PipelineCache cache) {
	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo()
		.setSetLayouts(allLayouts)
		.setPushConstantRanges(allPushConstants);

	if (blendAttachStates.empty()) {
		if (!allColourRenderingFormats.empty()) {
			for (int i = 0; i < allColourRenderingFormats.size(); ++i) {
				WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false);
			}
		}
		else {
			WithBlendState(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, false);
		}
	}

	blendCreate.setAttachments(blendAttachStates);
	blendCreate.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });

	VulkanPipeline output;

	output.layout = device.createPipelineLayoutUnique(pipeLayoutCreate);

	pipelineCreate.setPColorBlendState(&blendCreate)
		.setPDepthStencilState(&depthStencilCreate)
		.setPDynamicState(&dynamicCreate)
		.setPInputAssemblyState(&inputAsmCreate)
		.setPMultisampleState(&sampleCreate)
		.setPRasterizationState(&rasterCreate)
		.setLayout(*output.layout)
		.setPVertexInputState(&vertexCreate);
	//We must be using dynamic rendering, better set it up!
	vk::PipelineRenderingCreateInfoKHR			renderingCreate;
	if (!allColourRenderingFormats.empty() || depthRenderingFormat != vk::Format::eUndefined) {
		renderingCreate.depthAttachmentFormat		= depthRenderingFormat;
		renderingCreate.stencilAttachmentFormat		= stencilRenderingFormat;

		renderingCreate.colorAttachmentCount		= (uint32_t)allColourRenderingFormats.size();
		renderingCreate.pColorAttachmentFormats		= allColourRenderingFormats.data();

		pipelineCreate.pNext = &renderingCreate;
	}

	output.pipeline			= device.createGraphicsPipelineUnique(cache, pipelineCreate).value;

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::ePipeline, Vulkan::GetVulkanHandle(*output.pipeline), debugName);
	}

	return output;
}