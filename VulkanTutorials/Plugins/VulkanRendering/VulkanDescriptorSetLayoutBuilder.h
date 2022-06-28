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
		VulkanDescriptorSetLayoutBuilder(const std::string& name = "") { debugName = name; usingBindless = -1; };
		~VulkanDescriptorSetLayoutBuilder() {};

		VulkanDescriptorSetLayoutBuilder& WithSamplers(unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll);
		VulkanDescriptorSetLayoutBuilder& WithUniformBuffers(unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll);
		VulkanDescriptorSetLayoutBuilder& WithStorageBuffers(unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll);

		VulkanDescriptorSetLayoutBuilder& WithBindlessAccess();//Buffers after this are bindless!

		vk::UniqueDescriptorSetLayout Build(vk::Device device);

	protected:
		std::string	debugName;
		int usingBindless;
		std::vector< vk::DescriptorSetLayoutBinding> addedBindings;

		vk::DescriptorSetLayoutCreateInfo createInfo;
	};
}