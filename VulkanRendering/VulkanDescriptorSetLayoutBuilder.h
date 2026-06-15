/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering::Vulkan {
	class VulkanRenderer;

	/*
	DescriptorSetLayoutBuilder: This helper class can create new descriptor
	set layouts, using a fluent interface.
	
	*/
	class DescriptorSetLayoutBuilder {
	public:
		DescriptorSetLayoutBuilder(vk::Device m_device) {
			m_sourceDevice = m_device;
		};
		~DescriptorSetLayoutBuilder() {};

		DescriptorSetLayoutBuilder& WithDescriptor(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithDescriptor(vk::DescriptorType type, uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);

		DescriptorSetLayoutBuilder& WithUniformTexelBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithStorageTexelBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		
		DescriptorSetLayoutBuilder& WithSamplers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithImageSamplers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);	
		DescriptorSetLayoutBuilder& WithSampledImages(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithStorageImages(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);

		DescriptorSetLayoutBuilder& WithUniformBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithStorageBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);

		DescriptorSetLayoutBuilder& WithDynamicUniformBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);
		DescriptorSetLayoutBuilder& WithDynamicStorageBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);

		DescriptorSetLayoutBuilder& WithAccelStructures(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders = vk::ShaderStageFlagBits::eAll, vk::DescriptorBindingFlags = (vk::DescriptorBindingFlags)0);

		DescriptorSetLayoutBuilder& WithCreationFlags(vk::DescriptorSetLayoutCreateFlags flags);

		vk::UniqueDescriptorSetLayout Build(const std::string& debugName = "");

	protected:
		vk::Device m_sourceDevice;

		std::vector< vk::DescriptorSetLayoutBinding>	m_addedBindings;
		std::vector< vk::DescriptorBindingFlags>		m_addedFlags;

		vk::DescriptorSetLayoutCreateInfo m_createInfo;
	};
}