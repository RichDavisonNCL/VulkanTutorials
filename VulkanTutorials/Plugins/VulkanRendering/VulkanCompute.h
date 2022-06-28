/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering {
	class VulkanCompute	{
	public:
		VulkanCompute(vk::Device sourceDevice, const std::string& filename);
		~VulkanCompute() {}

		int GetThreadXCount() const;
		int GetThreadYCount() const;
		int GetThreadZCount() const;

		void	FillShaderStageCreateInfo(vk::ComputePipelineCreateInfo& info) const;

	protected:
		int localThreadSize[3];
		vk::PipelineShaderStageCreateInfo info;
		vk::UniqueShaderModule	computeModule;
	};
}