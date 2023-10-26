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
#include "VulkanBuffers.h"
namespace NCL::Maths {
	class Matrix4;
}

namespace NCL::Rendering::Vulkan {
	namespace BindingTableOrder {
		enum Type : uint32_t {
			RayGen,
			Hit,
			Miss,
			Call,
			MAX_SIZE
		};
	};

	struct ShaderBindingTable {
		VulkanBuffer tableBuffer;
		//address, stride, size
		vk::StridedDeviceAddressRegionKHR regions[4];
	};

	class VulkanShaderBindingTableBuilder {
	public:
		VulkanShaderBindingTableBuilder(const std::string& debugName = "");
		~VulkanShaderBindingTableBuilder();

		VulkanShaderBindingTableBuilder& WithProperties(vk::PhysicalDeviceRayTracingPipelinePropertiesKHR properties);

		VulkanShaderBindingTableBuilder& WithPipeline(vk::Pipeline pipe, const vk::RayTracingPipelineCreateInfoKHR& createInfo);

		VulkanShaderBindingTableBuilder& WithLibrary(const vk::RayTracingPipelineCreateInfoKHR& createInfo);

		ShaderBindingTable Build(vk::Device device, VmaAllocator allocator);

	protected:

		void FillIndices(const vk::RayTracingPipelineCreateInfoKHR* fromInfo, int& offset);

		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR properties;

		const vk::RayTracingPipelineCreateInfoKHR* pipeCreateInfo;

		std::vector<const vk::RayTracingPipelineCreateInfoKHR*> libraries;

		vk::Pipeline pipeline;

		std::string debugName;

		std::vector<uint32_t> handleIndices[BindingTableOrder::MAX_SIZE];
	};
}