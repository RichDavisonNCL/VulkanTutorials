/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

struct VulkanPipeline {
	vk::UniquePipeline			pipeline;
	vk::UniquePipelineLayout	layout;

	std::vector<std::vector<vk::DescriptorSetLayoutBinding>>	m_allLayoutsBindings;
	std::vector<vk::DescriptorSetLayout>						m_allLayouts;

	std::vector<vk::PushConstantRange>							m_pushConstants;

	std::vector<vk::UniqueDescriptorSetLayout>					m_createdLayouts;

	vk::DescriptorSetLayout GetSetLayout(uint32_t layout) const {
		//assert(index < m_allLayouts.size());
		//assert(!m_allLayouts.empty());
		return m_allLayouts[layout];
	}

	operator vk::Pipeline() const {
		return *pipeline;
	}

	operator vk::PipelineLayout() const {
		return *layout;
	}

};