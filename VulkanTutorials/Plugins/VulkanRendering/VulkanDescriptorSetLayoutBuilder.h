/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>
namespace NCL::Rendering {
	class VulkanRenderer;

	class VulkanDescriptorSetLayoutBuilder	{
	public:
		VulkanDescriptorSetLayoutBuilder(const std::string& name = "") { debugName = name; };
		~VulkanDescriptorSetLayoutBuilder() {};

		VulkanDescriptorSetLayoutBuilder& WithSamplers(unsigned int count, vk::ShaderStageFlags inShaders);
		VulkanDescriptorSetLayoutBuilder& WithUniformBuffers(unsigned int count, vk::ShaderStageFlags inShaders);

		VulkanDescriptorSetLayoutBuilder& WithStorageBuffers(unsigned int count, vk::ShaderStageFlags inShaders);

		VulkanDescriptorSetLayoutBuilder& WithDebugName(const std::string& name);

		vk::DescriptorSetLayout Build(vk::Device device);
		vk::UniqueDescriptorSetLayout BuildUnique(vk::Device device);

	protected:
		std::string	debugName;
		std::vector< vk::DescriptorSetLayoutBinding> addedBindings;
	};
}