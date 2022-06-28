/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanComputePipelineBuilder.h"
#include "VulkanCompute.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

VulkanComputePipelineBuilder::VulkanComputePipelineBuilder(const std::string& name) {
	debugName = name;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size) {
	allPushConstants.emplace_back(vk::PushConstantRange(flags, offset, size));
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithDescriptorSetLayout(vk::DescriptorSetLayout layout) {
	allLayouts.emplace_back(layout);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithShader(UniqueVulkanCompute& compute) {
	compute->FillShaderStageCreateInfo(pipelineCreate);
	return *this;
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::WithLayout(vk::PipelineLayout layout) {
	this->layout = layout;
	pipelineCreate.setLayout(layout);
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

	pipelineCreate.setLayout(*output.layout);

	output.pipeline = device.createComputePipelineUnique(cache, pipelineCreate).value;

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::ePipeline, Vulkan::GetVulkanHandle(*output.pipeline), debugName);
	}

	return output;
}