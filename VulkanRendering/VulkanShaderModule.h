/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/Shader.h"

namespace NCL::Rendering::Vulkan {
	class VulkanShaderModule  {
	public:
		VulkanShaderModule(const std::string& filename,  vk::ShaderStageFlagBits stage, vk::Device device);

		~VulkanShaderModule() = default;
		void CombineLayoutBindings(std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& inoutBindings)		const;
		void CombinePushConstantRanges(std::vector< vk::PushConstantRange>& inoutRanges)						const;

	//protected:		
		void AddReflectionData(uint32_t dataSize, const void* data, vk::ShaderStageFlags stage);	
		void BuildLayouts(vk::Device device);

		std::vector< vk::UniqueDescriptorSetLayout>	m_reflectionLayouts;
		std::vector<std::vector<vk::DescriptorSetLayoutBinding>>	m_allLayoutsBindings;

		std::vector< vk::PushConstantRange>		m_pushConstants;

		vk::UniqueShaderModule				m_shaderModule;
		vk::ShaderStageFlagBits				m_shaderStage;
	};
}