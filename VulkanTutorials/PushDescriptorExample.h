/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class PushDescriptorExample : public VulkanTutorialRenderer
	{
	public:
		PushDescriptorExample(Window& window);

		void SetupTutorial()	override;
		void RenderFrame()		override;

	protected:
		void SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) override;

		VulkanPipeline		pipeline;

		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;

		VulkanBuffer		uniformData[2];

		vk::UniqueDescriptorSetLayout	descriptorLayout;

		Vector4	colourUniform;
		Vector4 positionUniform;
	};
}

