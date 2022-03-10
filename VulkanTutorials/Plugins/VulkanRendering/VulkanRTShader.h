/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering {
	enum class RayTraceShaderStages {
		RayGen,
		AnyHit,
		ClosestHit,
		Miss,
		Intersection,
		MAXSIZE
	};

	class VulkanRTShader
	{
	public:

	};
}