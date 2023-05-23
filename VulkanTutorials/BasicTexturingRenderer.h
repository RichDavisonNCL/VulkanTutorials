/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class BasicTexturingRenderer : public VulkanTutorialRenderer	{
	public:
		BasicTexturingRenderer(Window& window);

		void SetupTutorial() override;

		void RenderFrame() override;

	protected:
		VulkanPipeline	texturePipeline;

		UniqueVulkanShader	defaultShader;
		UniqueVulkanMesh 		defaultMesh;
		UniqueVulkanTexture	textures[2];

		vk::UniqueDescriptorSet			descriptorSets[2];
		vk::UniqueDescriptorSetLayout	descriptorLayout;
	};
}