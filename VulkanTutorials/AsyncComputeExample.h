/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
    class AsyncComputeExample : public VulkanTutorial
    {
	public:
		AsyncComputeExample(Window& window);
		~AsyncComputeExample() {}
	protected:
		void RenderFrame(float dt) override;

		void	BuildRasterPipeline();
		void	BuildComputePipeline();

		UniqueVulkanShader	rasterShader;
		UniqueVulkanCompute	computeShader;

		VulkanPipeline	basicPipeline;
		VulkanPipeline	computePipeline;

		VulkanBuffer			frameIDBuffer;

		VulkanBuffer			particlePositions;
		vk::UniqueDescriptorSet	bufferDescriptor;

		vk::UniqueDescriptorSetLayout dataLayout;

		vk::UniqueCommandBuffer asyncBuffer;

		uint32_t frameID;
    };
}

