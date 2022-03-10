/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipeline.h"

namespace NCL::Rendering {
	class VulkanCompute;

	class VulkanComputePipelineBuilder	{
	public:
		VulkanComputePipelineBuilder(const std::string& name = "");
		~VulkanComputePipelineBuilder();

		VulkanComputePipelineBuilder& WithLayout(vk::PipelineLayout layout);

		VulkanComputePipelineBuilder& WithPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size);
		VulkanComputePipelineBuilder& WithPushConstant(vk::PushConstantRange layout);
		VulkanComputePipelineBuilder& WithDescriptorSetLayout(vk::DescriptorSetLayout layout);
		VulkanComputePipelineBuilder& WithComputeState(VulkanCompute* shader);
		VulkanComputePipelineBuilder& WithDebugName(const std::string& name);


		VulkanPipeline	Build(vk::Device device, vk::PipelineCache cache);
	protected:
		vk::PipelineLayout	layout;
		vk::ComputePipelineCreateInfo			pipelineCreate;

		std::vector<vk::DescriptorSetLayout>	allLayouts;
		std::vector<vk::PushConstantRange>		allPushConstants;

		std::string debugName;
	};
};