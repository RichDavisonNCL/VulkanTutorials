/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanMesh.h"
#include "VulkanShader.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::WithSamplers(unsigned int count, vk::ShaderStageFlags inShaders) {
	vk::DescriptorSetLayoutBinding binding = vk::DescriptorSetLayoutBinding()
		.setBinding((uint32_t)addedBindings.size())
		.setDescriptorCount(count)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setStageFlags(inShaders);

	addedBindings.emplace_back(binding);

	return *this;
}

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::WithUniformBuffers(unsigned int count, vk::ShaderStageFlags inShaders) {
	vk::DescriptorSetLayoutBinding binding = vk::DescriptorSetLayoutBinding()
		.setBinding((uint32_t)addedBindings.size())
		.setDescriptorCount(count)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setStageFlags(inShaders);

	addedBindings.emplace_back(binding);
	return *this;
}

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::WithStorageBuffers(unsigned int count, vk::ShaderStageFlags inShaders) {
	vk::DescriptorSetLayoutBinding binding = vk::DescriptorSetLayoutBinding()
		.setBinding((uint32_t)addedBindings.size())
		.setDescriptorCount(count)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setStageFlags(inShaders);

	addedBindings.emplace_back(binding);
	return *this;
}

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::WithBindlessAccess() {
	usingBindless = true;
	return *this;
}

vk::UniqueDescriptorSetLayout VulkanDescriptorSetLayoutBuilder::Build(vk::Device device) {
	createInfo.setBindings(addedBindings);
	vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo;
	std::vector< vk::DescriptorBindingFlags> bindingFlags;

	if (usingBindless >= 0) {
		for (int i = 0; i < addedBindings.size(); ++i) {
			bindingFlags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind);
		}
		bindingFlagsInfo.setBindingFlags(bindingFlags);
		createInfo.pNext = &bindingFlagsInfo;

		createInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
	}
	

	vk::UniqueDescriptorSetLayout layout = std::move(device.createDescriptorSetLayoutUnique(createInfo));
	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eDescriptorSetLayout, Vulkan::GetVulkanHandle(*layout), debugName);
	}
	return layout;
}