/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

struct VulkanPipeline {
	vk::UniquePipeline			pipeline;
	vk::UniquePipelineLayout	layout;
};