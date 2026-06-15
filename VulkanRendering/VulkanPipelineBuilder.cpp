/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanPipelineBuilder.h"
#include "VulkanMesh.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

PipelineBuilder::PipelineBuilder(vk::Device device) : PipelineBuilderBase(device)	{
	m_ignoreDynamicDefaults = false;

	m_sampleCreate.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	m_viewportCreate.setViewportCount(1);
	m_viewportCreate.setScissorCount(1);

	m_pipelineCreate.setPViewportState(&m_viewportCreate);

	m_depthStencilCreate.setDepthCompareOp(vk::CompareOp::eAlways)
		.setDepthTestEnable(false)
		.setDepthWriteEnable(false)
		.setStencilTestEnable(false)
		.setDepthBoundsTestEnable(false);

	m_depthRenderingFormat		= vk::Format::eUndefined;

	m_rasterCreate.setCullMode(vk::CullModeFlagBits::eNone)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setLineWidth(1.0f);

	m_inputAsmCreate.setTopology(vk::PrimitiveTopology::eTriangleList);
}

PipelineBuilder& PipelineBuilder::WithRasterState(vk::CullModeFlagBits cullMode, vk::PolygonMode polyMode) {
	m_rasterCreate.setCullMode(cullMode).setPolygonMode(polyMode);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithRasterState(const vk::PipelineRasterizationStateCreateInfo& info) {
	m_rasterCreate = info;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithVertexInputState(const vk::PipelineVertexInputStateCreateInfo& spec) {
	m_vertexCreate = spec;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithTopology(vk::PrimitiveTopology topology, bool primitiveRestart) {
	m_inputAsmCreate.setTopology(topology).setPrimitiveRestartEnable(primitiveRestart);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithPass(vk::RenderPass& renderPass) {
	m_pipelineCreate.setRenderPass(renderPass);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithLayout(vk::PipelineLayout& m_layout) {
	m_externalLayout = m_layout;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithColourAttachment(vk::Format f) {
	m_allColourRenderingFormats.push_back(f);

	vk::PipelineColorBlendAttachmentState pipeBlend;
	pipeBlend.setBlendEnable(false);
	pipeBlend.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	m_blendAttachStates.emplace_back(pipeBlend);

	return *this;
}

PipelineBuilder& PipelineBuilder::WithColourAttachment(vk::Format f, vk::BlendFactor srcState, vk::BlendFactor dstState) {
	m_allColourRenderingFormats.push_back(f);

	vk::PipelineColorBlendAttachmentState pipeBlend;

	pipeBlend.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(true)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorBlendOp(vk::BlendOp::eAdd)

		.setSrcAlphaBlendFactor(srcState)
		.setSrcColorBlendFactor(srcState)

		.setDstAlphaBlendFactor(dstState)
		.setDstColorBlendFactor(dstState);

	m_blendAttachStates.emplace_back(pipeBlend);

	return *this;
}

PipelineBuilder& PipelineBuilder::WithColourAttachment(vk::Format f, vk::PipelineColorBlendAttachmentState state) {
	m_allColourRenderingFormats.push_back(f);
	m_blendAttachStates.emplace_back(state);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithoutDefaultDynamicState() {
	m_ignoreDynamicDefaults = true;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithDynamicState(vk::DynamicState state) {
	m_dynamicStates.push_back(state);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithDepthAttachment(vk::Format depthFormat) {
	m_depthRenderingFormat = depthFormat;

	return *this;
}

PipelineBuilder& PipelineBuilder::WithDepthAttachment(vk::Format depthFormat, vk::CompareOp op, bool testEnabled, bool writeEnabled) {
	m_depthRenderingFormat = depthFormat;
	m_depthStencilCreate.setDepthCompareOp(op)
		.setDepthTestEnable(testEnabled)
		.setDepthWriteEnable(writeEnabled);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithDepthAttachment(vk::Format depthFormat, vk::PipelineDepthStencilStateCreateInfo& info) {
	m_depthRenderingFormat	= depthFormat;
	m_depthStencilCreate		= info;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithStencilOps(vk::StencilOpState state) {
	m_depthStencilCreate.front = state;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithStencilOpsFront(vk::StencilOpState state) {
	m_depthStencilCreate.front = state;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithStencilOpsBack(vk::StencilOpState state) {
	m_depthStencilCreate.back = state;
	return *this;
}

PipelineBuilder& PipelineBuilder::WithTessellationPatchVertexCount(uint32_t controlPointsPerPatch) {
	m_tessellationCreate.setPatchControlPoints(controlPointsPerPatch);
	m_pipelineCreate.setPTessellationState(&m_tessellationCreate);
	return *this;
}

PipelineBuilder& PipelineBuilder::WithShaderBinary(const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entrypoint) {
	m_loadedShaderModules.push_back(std::make_unique<VulkanShaderModule>(filename, stage, m_sourceDevice));
	m_usedModules.push_back(m_loadedShaderModules.back().get());
	m_moduleEntryPoints.push_back(entrypoint);
	return *this;
}


PipelineBuilder& PipelineBuilder::WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint) {
	m_usedModules.push_back(&module);
	m_moduleEntryPoints.push_back(entrypoint);
	return *this;
}

VulkanPipeline	PipelineBuilder::Build(const std::string& debugName, vk::PipelineCache cache) {
	m_blendCreate.setAttachments(m_blendAttachStates);
	m_blendCreate.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });

	if (!m_ignoreDynamicDefaults)	{
		m_dynamicStates.push_back(vk::DynamicState::eViewport);
		m_dynamicStates.push_back(vk::DynamicState::eScissor);
	}		

	m_dynamicCreate.setDynamicStateCount(m_dynamicStates.size());
	m_dynamicCreate.setPDynamicStates(m_dynamicStates.data());

	vk::Format stencilRenderingFormat = vk::Format::eUndefined; //TODO

	VulkanPipeline output;

	m_pipelineCreate.setPColorBlendState(&m_blendCreate)
		.setPDepthStencilState(&m_depthStencilCreate)
		.setPDynamicState(&m_dynamicCreate)
		.setPInputAssemblyState(&m_inputAsmCreate)
		.setPMultisampleState(&m_sampleCreate)
		.setPRasterizationState(&m_rasterCreate)
		.setPVertexInputState(&m_vertexCreate);

	FillShaderState(output);

	//We must be using dynamic rendering, better set it up!
	if (!m_allColourRenderingFormats.empty() || m_depthRenderingFormat != vk::Format::eUndefined) {
		m_renderingCreate.depthAttachmentFormat		= m_depthRenderingFormat;
		m_renderingCreate.stencilAttachmentFormat	= stencilRenderingFormat;

		m_renderingCreate.colorAttachmentCount		= (uint32_t)m_allColourRenderingFormats.size();
		m_renderingCreate.pColorAttachmentFormats	= m_allColourRenderingFormats.data();

		m_renderingCreate.pNext = m_pipelineCreate.pNext;
		m_pipelineCreate.pNext	= &m_renderingCreate;
	}

	vk::PipelineCreateFlags2CreateInfo pipeFlags;
	if (m_pipelineCreateBits) {
		pipeFlags.flags			= m_pipelineCreateBits;
		pipeFlags.pNext			= m_pipelineCreate.pNext;
		m_pipelineCreate.pNext	= &pipeFlags;
	}

	output.pipeline			= m_sourceDevice.createGraphicsPipelineUnique(cache, m_pipelineCreate).value;

	if (!debugName.empty()) {
		SetDebugName(m_sourceDevice, *output.pipeline	, debugName);
		SetDebugName(m_sourceDevice, *output.layout		, debugName);
	}

	return output;
}
