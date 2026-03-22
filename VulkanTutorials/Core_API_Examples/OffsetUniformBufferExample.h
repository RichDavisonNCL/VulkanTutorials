/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class OffsetUniformBufferExample : public VulkanTutorial {
	public:
		OffsetUniformBufferExample(Window& window, VulkanInitialisation& vkInit);
		~OffsetUniformBufferExample() = default;

	protected:
		void RenderFrame(float dt) override;
		VulkanPipeline		pipeline;

		vk::UniqueDescriptorSet			objectMatrixSet;
		vk::UniqueDescriptorSetLayout	objectMatrixLayout;

		VulkanBuffer	objectMatrixData;
	};
}