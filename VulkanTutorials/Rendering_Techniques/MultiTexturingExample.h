/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class MultiTexturingExample : public VulkanTutorial	{
	public:
		MultiTexturingExample(Window& window, VulkanInitialisation& vkInit);
	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline	pipeline;

		UniqueVulkanShader	shader;
		UniqueVulkanMesh 		mesh;
		UniqueVulkanTexture	textures[4];

		vk::UniqueDescriptorSet			descriptorSets[2];
	};
}