/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class DescriptorBufferExample : public VulkanTutorial
	{
	public:
		DescriptorBufferExample(Window& window);

	protected:
		void	RenderFrame(float dt) override;

		VulkanPipeline	pipeline;

		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;

		VulkanBuffer	uniformData[2];
		vk::UniqueDescriptorSet			descriptorSet;
		vk::UniqueDescriptorSetLayout	descriptorLayout;

		VulkanBuffer		descriptorBuffer;
		vk::DeviceSize		descriptorBufferSize;

		Vector4	colourUniform;
		Vector4 positionUniform;

		vk::PhysicalDeviceDescriptorBufferPropertiesEXT descriptorProperties;
	};
}

