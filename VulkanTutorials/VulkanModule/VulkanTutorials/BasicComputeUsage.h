/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class BasicComputeUsage : public VulkanTutorialRenderer {
	public:
		BasicComputeUsage(Window& window);
		~BasicComputeUsage();

	protected:
		virtual void RenderFrame();

		void	BuildRasterPipeline();
		void	BuildComputePipeline();

		std::shared_ptr<VulkanMesh>		particleMesh;

		std::shared_ptr<VulkanShader>	rasterShader;
		std::shared_ptr<VulkanCompute>	computeShader;

		VulkanPipeline	basicPipeline;
		VulkanPipeline	computePipeline;

		VulkanBuffer		particlePositions;
		vk::DescriptorSet	bufferDescriptor;
	};
}