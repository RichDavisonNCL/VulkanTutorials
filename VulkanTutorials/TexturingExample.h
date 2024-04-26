/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class TexturingExample : public VulkanTutorial	{
	public:
		TexturingExample(Window& window);
	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline	pipeline;

		UniqueVulkanShader	shader;
		UniqueVulkanMesh 		mesh;
		UniqueVulkanTexture	textures[2];

		vk::UniqueDescriptorSet			descriptorSets[2];
	};
}