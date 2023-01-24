/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class DescriptorBufferExample : public VulkanTutorialRenderer
	{
	public:
		DescriptorBufferExample(Window& window);

		void SetupTutorial() override;

		void RenderFrame() override;

	protected:
		void SetupDeviceInfo(vk::DeviceCreateInfo& info) override;

		VulkanPipeline	pipeline;

		UniqueVulkanMesh 		triMesh;
		UniqueVulkanShader	shader;

		VulkanBuffer	uniformData[2];
		vk::DeviceAddress uniformDataAddresses[2];
		vk::UniqueDescriptorSet			descriptorSet;
		vk::UniqueDescriptorSetLayout	descriptorLayout;

		VulkanBuffer		descriptorBuffer;
		vk::DeviceSize		descriptorBufferSize;
		vk::DeviceAddress	descriptorBufferAddress;
		//void*				descriptorBufferMemory;

		Vector4	colourUniform;
		Vector4 positionUniform;

		vk::PhysicalDeviceDescriptorBufferPropertiesEXT descriptorProperties;
	};
}

