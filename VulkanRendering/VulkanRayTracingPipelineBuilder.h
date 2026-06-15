/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../VulkanRendering/VulkanRenderer.h"
#include "../VulkanRendering/VulkanPipelineBuilderBase.h"

namespace NCL::Rendering::Vulkan {

	class VulkanRayTracingPipelineBuilder : 
		public PipelineBuilderBase< VulkanRayTracingPipelineBuilder, vk::RayTracingPipelineCreateInfoKHR> {
	public:
		VulkanRayTracingPipelineBuilder(vk::Device m_device);
		~VulkanRayTracingPipelineBuilder();

		VulkanRayTracingPipelineBuilder& WithShaderBinary(const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entrypoint = "main");
		VulkanRayTracingPipelineBuilder& WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint = "main");

		VulkanRayTracingPipelineBuilder& WithRayGenGroup(uint32_t shaderIndex);
		VulkanRayTracingPipelineBuilder& WithMissGroup(uint32_t shaderIndex);
		VulkanRayTracingPipelineBuilder& WithTriangleHitGroup(uint32_t closestHit = VK_SHADER_UNUSED_KHR, uint32_t anyHit = VK_SHADER_UNUSED_KHR);
		VulkanRayTracingPipelineBuilder& WithProceduralHitGroup(uint32_t intersection, uint32_t closestHit = VK_SHADER_UNUSED_KHR, uint32_t anyHit = VK_SHADER_UNUSED_KHR);

		VulkanRayTracingPipelineBuilder& WithRecursionDepth(uint32_t count);

		VulkanPipeline Build(const std::string& debugName = "", vk::PipelineCache cache = {});

	protected:
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_genGroups;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_missGroups;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_hitGroups;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_allGroups;

		vk::PipelineDynamicStateCreateInfo					m_dynamicCreate;
	};
}