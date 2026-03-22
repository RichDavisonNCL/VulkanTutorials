/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class PushConstantExample : public VulkanTutorial {
	public:
		PushConstantExample(Window& window, VulkanInitialisation& vkInit);
		~PushConstantExample() = default;

	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline	pipeline;

		Vector4	colourUniform;
		Vector3 positionUniform;
	};
}