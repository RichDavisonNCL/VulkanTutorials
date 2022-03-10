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
#include "Vulkan.h"

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

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::WithDebugName(const string& name) {
	debugName = name;
	return *this;
}

vk::DescriptorSetLayout VulkanDescriptorSetLayoutBuilder::Build(vk::Device device) {
	vk::DescriptorSetLayout outLayout = device.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo({}, (uint32_t)addedBindings.size(), addedBindings.data())
	);	
	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eDescriptorSetLayout, (uint64_t)(VkDescriptorSetLayout)outLayout, debugName);
	}
	return outLayout;
}

vk::UniqueDescriptorSetLayout VulkanDescriptorSetLayoutBuilder::BuildUnique(vk::Device device) {
	vk::UniqueDescriptorSetLayout layout = std::move(device.createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo({}, (uint32_t)addedBindings.size(), addedBindings.data())
	));
	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eDescriptorSetLayout, (uint64_t)(VkDescriptorSetLayout)layout.get(), debugName);
	}
	return layout;
}