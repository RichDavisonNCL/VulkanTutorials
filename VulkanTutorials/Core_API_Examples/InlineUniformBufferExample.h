/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class InlineUniformBufferExample : public VulkanTutorial {
	public:
		InlineUniformBufferExample(Window& window, VulkanInitialisation& vkInit);
		~InlineUniformBufferExample() = default;

	protected:
		void RenderFrame(float dt) override;
		void	UpdateCameraUniform(vk::CommandBuffer buffer);

		VulkanPipeline		pipeline;

		vk::UniqueDescriptorSet			descriptorSet;

		PerspectiveCamera	camera;

		vk::UniqueDescriptorSetLayout inlineLayout;
	};
}