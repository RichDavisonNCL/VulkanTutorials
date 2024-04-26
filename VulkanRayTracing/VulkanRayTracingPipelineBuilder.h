/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../VulkanRendering/VulkanRenderer.h"
#include "../VulkanRendering/VulkanPipelineBuilderBase.h"
#include "VulkanRTShader.h"

namespace NCL::Rendering::Vulkan {

	class VulkanRayTracingPipelineBuilder : 
		public PipelineBuilderBase< VulkanRayTracingPipelineBuilder, vk::RayTracingPipelineCreateInfoKHR> {
	public:
		VulkanRayTracingPipelineBuilder(vk::Device device);
		~VulkanRayTracingPipelineBuilder();

		VulkanRayTracingPipelineBuilder& WithShader(VulkanRTShader& shader, vk::ShaderStageFlagBits stage, const string& entry = "main");

		VulkanRayTracingPipelineBuilder& WithShaderGroup(const vk::RayTracingShaderGroupCreateInfoKHR& group);

		VulkanRayTracingPipelineBuilder& WithGeneralGroup(uint32_t index);
		VulkanRayTracingPipelineBuilder& WithTriangleHitGroup(uint32_t closestHit = VK_SHADER_UNUSED_KHR, uint32_t anyHit = VK_SHADER_UNUSED_KHR);
		VulkanRayTracingPipelineBuilder& WithProceduralHitGroup(uint32_t intersection, uint32_t closestHit = VK_SHADER_UNUSED_KHR, uint32_t anyHit = VK_SHADER_UNUSED_KHR);

		VulkanRayTracingPipelineBuilder& WithRecursionDepth(uint32_t count);

		VulkanPipeline Build(const std::string& debugName = "", vk::PipelineCache cache = {});

	protected:
		struct ShaderEntry {
			std::string				entryPoint;
			VulkanRTShader*			shader;
			vk::ShaderStageFlagBits	stage;
		};

		std::vector<ShaderEntry> entries;

		//std::vector<vk::PipelineShaderStageCreateInfo>		shaderEntries;
		std::vector<vk::PipelineShaderStageCreateInfo>		shaderStages;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

		vk::PipelineDynamicStateCreateInfo			dynamicCreate;
	};
}