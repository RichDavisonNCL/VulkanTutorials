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
	namespace BindingTableOrder {
		enum Type : uint32_t {
			RayGen,
			Miss,
			Hit,
			Call,
			MAX_SIZE
		};
	};

	struct ShaderBindingTable {
		VulkanBuffer tableBuffer;
		vk::StridedDeviceAddressRegionKHR regions[BindingTableOrder::MAX_SIZE];
	};

	class VulkanShaderBindingTableBuilder {
	public:
		VulkanShaderBindingTableBuilder(const std::string& debugName = "");
		~VulkanShaderBindingTableBuilder() = default;

		VulkanShaderBindingTableBuilder& WithProperties(vk::PhysicalDeviceRayTracingPipelinePropertiesKHR properties);

		VulkanShaderBindingTableBuilder& WithPipeline(vk::Pipeline pipe, const vk::RayTracingPipelineCreateInfoKHR& createInfo);

		VulkanShaderBindingTableBuilder& WithLibrary(const vk::RayTracingPipelineCreateInfoKHR& createInfo);

		ShaderBindingTable Build(vk::Device device, VulkanMemoryManager& memManager);

	protected:
		void FillCounts(const vk::RayTracingPipelineCreateInfoKHR* fromInfo);

		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR properties;

		const vk::RayTracingPipelineCreateInfoKHR* pipeCreateInfo;

		std::vector<const vk::RayTracingPipelineCreateInfoKHR*> libraries;

		vk::Pipeline pipeline;

		std::string debugName;

		uint32_t handleCounts[BindingTableOrder::MAX_SIZE] = { };
	};
}