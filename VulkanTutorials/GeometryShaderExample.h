/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"
namespace NCL::Rendering {
	class GeometryShaderExample : public VulkanTutorialRenderer
	{
	public:
		GeometryShaderExample(Window& window);

		void SetupTutorial() override;

		void RenderFrame() override;
	protected:
		void SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) override;

		VulkanPipeline		pipeline;
		UniqueVulkanShader	shader;
		UniqueVulkanMesh 	mesh;
	};
}