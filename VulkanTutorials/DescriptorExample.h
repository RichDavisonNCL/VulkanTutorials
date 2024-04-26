/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class DescriptorExample : public VulkanTutorial	{
	public:
		DescriptorExample(Window& window);
		~DescriptorExample() {}

	protected:
		void RenderFrame(float dt) override;

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