/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanRTShader.h"

vk::ShaderStageFlagBits rayTraceStageTypes[] = {
	vk::ShaderStageFlagBits::eRaygenKHR,
	vk::ShaderStageFlagBits::eAnyHitKHR,
	vk::ShaderStageFlagBits::eClosestHitKHR,
	vk::ShaderStageFlagBits::eMissKHR,
	vk::ShaderStageFlagBits::eIntersectionKHR
};