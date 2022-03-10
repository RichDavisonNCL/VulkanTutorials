/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanComputePipelineBuilder.h"
#include "VulkanCompute.h"
#include "Vulkan.h"

using namespace NCL;
using namespace Rendering;

VulkanComputePipelineBuilder::VulkanComputePipelineBuilder(const std::string& name) {
	debugName = name;
}

VulkanComputePipelineBuilder::~VulkanComputePipelineBuilder() {
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size) {
	allPushConstants.emplace_back(vk::PushConstantRange(flags, offset, size));
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithPushConstant(vk::PushConstantRange layout) {
	allPushConstants.emplace_back(layout);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithDescriptorSetLayout(vk::DescriptorSetLayout layout) {
	allLayouts.emplace_back(layout);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithComputeState(VulkanCompute* compute) {
	compute->FillShaderStageCreateInfo(pipelineCreate);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithLayout(vk::PipelineLayout layout) {
	this->layout = layout;
	pipelineCreate.setLayout(layout);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithDebugName(const std::string& name) {
	debugName = name;
	return *this;
}

VulkanPipeline	VulkanComputePipelineBuilder::Build(vk::Device device, vk::PipelineCache cache) {
	VulkanPipeline output;

	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount((uint32_t)allLayouts.size())
		.setPSetLayouts(allLayouts.data())
		.setPPushConstantRanges(allPushConstants.data())
		.setPushConstantRangeCount((uint32_t)allPushConstants.size());

	output.layout = device.createPipelineLayoutUnique(pipeLayoutCreate);

	pipelineCreate.setLayout(output.layout.get());

	output.pipeline = device.createComputePipelineUnique(cache, pipelineCreate).value;

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)output.pipeline.get(), debugName);
	}

	return output;
}