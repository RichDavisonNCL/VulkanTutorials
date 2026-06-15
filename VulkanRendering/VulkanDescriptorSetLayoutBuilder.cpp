/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanMesh.h"
#include "Vulkanrenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithDescriptor(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBindingFlags bindingFlags) {
	m_addedBindings.emplace_back(binding);
	m_addedFlags.emplace_back(bindingFlags);

	return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithDescriptor(vk::DescriptorType type, uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	vk::DescriptorSetLayoutBinding binding = {
		.binding			= index,
		.descriptorType		= type,
		.descriptorCount	= count,
		.stageFlags			= inShaders,
	};

	m_addedBindings.emplace_back(binding);
	m_addedFlags.emplace_back(bindingFlags);

	return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithDescriptors(const std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorBindingFlags flags) {
	m_addedBindings = bindings;
	m_addedFlags = std::vector< vk::DescriptorBindingFlags>(m_addedBindings.size(), flags);

	return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithSamplers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eSampler, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithUniformTexelBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eUniformTexelBuffer, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithStorageTexelBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eStorageTexelBuffer, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithImageSamplers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eCombinedImageSampler, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithSampledImages(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eSampledImage, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithStorageImages(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eStorageImage, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithUniformBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eUniformBuffer, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithStorageBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eStorageBuffer, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithDynamicUniformBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eUniformBufferDynamic, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithDynamicStorageBuffers(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eStorageBufferDynamic, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithAccelStructures(uint32_t index, unsigned int count, vk::ShaderStageFlags inShaders, vk::DescriptorBindingFlags bindingFlags) {
	return WithDescriptor(vk::DescriptorType::eAccelerationStructureKHR, index, count, inShaders, bindingFlags);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::WithCreationFlags(vk::DescriptorSetLayoutCreateFlags flags) {
	m_createInfo.flags |= flags;
	return *this;
}

vk::UniqueDescriptorSetLayout DescriptorSetLayoutBuilder::Build(const std::string& debugName) {
	m_createInfo.setBindings(m_addedBindings);
	vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo;
	
	bindingFlagsInfo.setBindingFlags(m_addedFlags);

	m_createInfo.pNext = &bindingFlagsInfo;
	vk::UniqueDescriptorSetLayout m_layout = std::move(m_sourceDevice.createDescriptorSetLayoutUnique(m_createInfo));
	if (!debugName.empty()) {
		SetDebugName(m_sourceDevice, *m_layout, debugName);
	}
	return m_layout;
}