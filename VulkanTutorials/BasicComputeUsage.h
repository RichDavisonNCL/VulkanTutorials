/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class BasicComputeUsage : public VulkanTutorialRenderer {
	public:
		BasicComputeUsage(Window& window);
		~BasicComputeUsage() {}

		void SetupTutorial() override;

	protected:
		virtual void RenderFrame();

		void	BuildRasterPipeline();
		void	BuildComputePipeline();

		UniqueVulkanShader	rasterShader;
		UniqueVulkanCompute	computeShader;

		VulkanPipeline	basicPipeline;
		VulkanPipeline	computePipeline;

		VulkanBuffer			particlePositions;
		vk::UniqueDescriptorSet	bufferDescriptor;

		vk::UniqueDescriptorSetLayout dataLayout;
	};
}