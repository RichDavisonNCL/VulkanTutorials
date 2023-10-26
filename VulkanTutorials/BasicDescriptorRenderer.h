/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class BasicDescriptorRenderer : public VulkanTutorialRenderer	{
	public:
		BasicDescriptorRenderer(Window& window);
		~BasicDescriptorRenderer() {}

		void SetupTutorial() override;

		void RenderFrame() override;

	protected:
		VulkanPipeline	pipeline;

		UniqueVulkanMesh 		triMesh;
		UniqueVulkanShader	shader;

		VulkanBuffer	uniformData[2];

		vk::UniqueDescriptorSet			descriptorSet;
		vk::UniqueDescriptorSetLayout	descriptorLayout;

		Vector4	colourUniform;
		Vector3 positionUniform;
	};
}