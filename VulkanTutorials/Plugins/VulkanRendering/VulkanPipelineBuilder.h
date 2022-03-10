/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipeline.h"

namespace NCL::Rendering {
	class VulkanRenderer;
	class VulkanShader;

	struct VulkanVertexSpecification;

	class VulkanPipelineBuilder	{
	public:
		VulkanPipelineBuilder(const std::string& debugName = "");
		~VulkanPipelineBuilder();

		VulkanPipelineBuilder& WithDepthState(vk::CompareOp op, bool depthEnabled, bool writeEnabled, bool stencilEnabled = false);

		VulkanPipelineBuilder& WithBlendState(vk::BlendFactor srcState, vk::BlendFactor dstState, bool enabled = true);

		VulkanPipelineBuilder& WithRaster(vk::CullModeFlagBits cullMode, vk::PolygonMode polyMode = vk::PolygonMode::eFill);

		VulkanPipelineBuilder& WithVertexSpecification(const vk::PipelineVertexInputStateCreateInfo& spec);

		VulkanPipelineBuilder& WithTopology(vk::PrimitiveTopology topology);

		VulkanPipelineBuilder& WithShaderState(const VulkanShader* shader);
		VulkanPipelineBuilder& WithShaderState(const std::unique_ptr<VulkanShader>& shader);

		VulkanPipelineBuilder& WithLayout(vk::PipelineLayout layout);

		VulkanPipelineBuilder& WithPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size);
		VulkanPipelineBuilder& WithPushConstant(vk::PushConstantRange layout);

		VulkanPipelineBuilder& WithDescriptorSetLayout(vk::DescriptorSetLayout layout);
		VulkanPipelineBuilder& WithDescriptorSetLayout(const vk::UniqueDescriptorSetLayout& layout);

		VulkanPipelineBuilder& WithPass(vk::RenderPass& renderPass);

		VulkanPipelineBuilder& WithDebugName(const std::string& name);

		VulkanPipelineBuilder& WithDepthStencilFormat(vk::Format depthFormat, vk::Format stencilFormat = vk::Format::eUndefined);
		VulkanPipelineBuilder& WithColourFormats(const std::vector<vk::Format>& formats);

		VulkanPipeline	Build(vk::Device device, vk::PipelineCache cache);

	protected:
		vk::GraphicsPipelineCreateInfo				pipelineCreate;
		vk::PipelineCacheCreateInfo					cacheCreate;
		vk::PipelineInputAssemblyStateCreateInfo	inputAsmCreate;
		vk::PipelineRasterizationStateCreateInfo	rasterCreate;
		vk::PipelineColorBlendStateCreateInfo		blendCreate;
		vk::PipelineDepthStencilStateCreateInfo		depthStencilCreate;
		vk::PipelineViewportStateCreateInfo			viewportCreate;
		vk::PipelineMultisampleStateCreateInfo		sampleCreate;
		vk::PipelineDynamicStateCreateInfo			dynamicCreate;

		vk::PipelineLayout layout;

		std::vector< vk::PipelineColorBlendAttachmentState>			blendAttachStates;

		vk::DynamicState dynamicStateEnables[2];

		std::vector< vk::DescriptorSetLayout> allLayouts;
		std::vector< vk::PushConstantRange> allPushConstants;

		std::vector<vk::Format> allColourRenderingFormats;
		vk::Format depthRenderingFormat;
		vk::Format stencilRenderingFormat;

		std::string debugName;
	};
}