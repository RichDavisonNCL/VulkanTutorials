/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipelineBuilderBase.h"
#include "VulkanPipeline.h"

namespace NCL::Rendering::Vulkan {
	class VulkanRenderer;
	class VulkanShader;
	using UniqueVulkanShader = std::unique_ptr<VulkanShader>;

	struct VulkanVertexSpecification;
	/*
	PipelineBuilder: Builder class for rasterisation pipelines. 
	The builder can automatically obtain descriptor set layouts and
	push constants from the shader binary, so these don't have to be
	manually added. By default, all pipelines will have two dynamic
	states - the viewport and scissor region. The reasoning behind this
	is that a) it means we don't have to recreate pipelines if the screen
	is resized, and b) these both appear to be zero-cost on every platform
	worth thinking about. 
	*/
	class PipelineBuilder	: public PipelineBuilderBase<PipelineBuilder, vk::GraphicsPipelineCreateInfo> {
	public:
		PipelineBuilder(vk::Device m_device);
		~PipelineBuilder() {}

		PipelineBuilder& WithRasterState(vk::CullModeFlagBits cullMode, vk::PolygonMode polyMode = vk::PolygonMode::eFill);
		PipelineBuilder& WithRasterState(const vk::PipelineRasterizationStateCreateInfo& info);

		PipelineBuilder& WithVertexInputState(const vk::PipelineVertexInputStateCreateInfo& spec);

		PipelineBuilder& WithTessellationPatchVertexCount(uint32_t controlPointsPerPatch);

		PipelineBuilder& WithTopology(vk::PrimitiveTopology topology, bool primitiveRestart = false);
		PipelineBuilder& WithPass(vk::RenderPass& renderPass);

		PipelineBuilder& WithLayout(vk::PipelineLayout& m_layout);

		//Depth attachment that does nothing?
		PipelineBuilder& WithDepthAttachment(vk::Format depthFormat);
		//Depth attachment with standard settings
		PipelineBuilder& WithDepthAttachment(vk::Format depthFormat, vk::CompareOp op, bool testEnabled, bool writeEnable);
		//Depth attachment with user-defined settings
		PipelineBuilder& WithDepthAttachment(vk::Format depthFormat, vk::PipelineDepthStencilStateCreateInfo& info);

		PipelineBuilder& WithStencilOps(vk::StencilOpState state);
		PipelineBuilder& WithStencilOpsFront(vk::StencilOpState state);
		PipelineBuilder& WithStencilOpsBack(vk::StencilOpState state);

		//A colour attachment, no blending
		PipelineBuilder& WithColourAttachment(vk::Format f); 
		//A colour attachment, with blending
		PipelineBuilder& WithColourAttachment(vk::Format f, vk::BlendFactor srcState, vk::BlendFactor dstState);
		//A colour attachment, with user-defined state
		PipelineBuilder& WithColourAttachment(vk::Format f, vk::PipelineColorBlendAttachmentState state);

		//By default, pipelines have a dynamic viewport and scissor region.
		//If you really want to, you can disable that here.
		PipelineBuilder& WithoutDefaultDynamicState();

		PipelineBuilder& WithDynamicState(vk::DynamicState state);

		PipelineBuilder& WithShaderBinary(const std::string& filename , vk::ShaderStageFlagBits stage, const std::string& entrypoint = "main");
		PipelineBuilder& WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint = "main");

		VulkanPipeline	Build(const std::string& debugName = "", vk::PipelineCache cache = {});

		vk::PipelineRenderingCreateInfoKHR& GetRenderingCreateInfo()  {
			return m_renderingCreate;
		}

	protected:
		vk::PipelineCacheCreateInfo					m_cacheCreate;
		vk::PipelineInputAssemblyStateCreateInfo	m_inputAsmCreate;
		vk::PipelineRasterizationStateCreateInfo	m_rasterCreate;
		vk::PipelineColorBlendStateCreateInfo		m_blendCreate;
		vk::PipelineDepthStencilStateCreateInfo		m_depthStencilCreate;
		vk::PipelineViewportStateCreateInfo			m_viewportCreate;
		vk::PipelineMultisampleStateCreateInfo		m_sampleCreate;
		vk::PipelineDynamicStateCreateInfo			m_dynamicCreate;
		vk::PipelineVertexInputStateCreateInfo		m_vertexCreate;
		vk::PipelineTessellationStateCreateInfo		m_tessellationCreate;

		vk::PipelineRenderingCreateInfoKHR			m_renderingCreate;

		std::vector< vk::PipelineColorBlendAttachmentState>			m_blendAttachStates;

		std::vector<vk::DynamicState> m_dynamicStates;

		std::vector<vk::Format> m_allColourRenderingFormats;
		vk::Format m_depthRenderingFormat;

		bool m_ignoreDynamicDefaults;
	};
}