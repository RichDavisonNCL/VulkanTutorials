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
		AsyncComputeExample(Window& window, VulkanInitialisation& vkInit);
		~AsyncComputeExample();
	protected:
		void RenderFrame(float dt) override;

		UniqueVulkanShader	rasterShader;
		UniqueVulkanCompute	computeShader;

		VulkanPipeline	rasterPipeline;
		VulkanPipeline	computePipeline;

		VulkanBuffer			frameIDBuffer;

		VulkanBuffer			particlePositions;
		vk::UniqueDescriptorSet	bufferDescriptor;

		vk::UniqueDescriptorSetLayout dataLayout;

		vk::UniqueCommandBuffer asyncBuffers[2];
		vk::UniqueFence			asyncFences[2];

		uint32_t frameID;
    };
}

