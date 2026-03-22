/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class PushDescriptorExample : public VulkanTutorial
	{
	public:
		PushDescriptorExample(Window& window, VulkanInitialisation& vkInit);
		~PushDescriptorExample() = default;

	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline		pipeline;

		VulkanBuffer	uniformData[2];

		Vector4	colourUniform;
		Vector4 positionUniform;
	};
}

