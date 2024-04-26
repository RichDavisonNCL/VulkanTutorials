/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class MultiPipelineExample : public VulkanTutorial {
	public:
		MultiPipelineExample(Window& window);
		~MultiPipelineExample() {}

	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline	solidColourPipeline;
		VulkanPipeline	texturedPipeline;
		UniqueVulkanMesh	triangleMesh;

		UniqueVulkanTexture	textures[2];

		UniqueVulkanShader texturingShader;
		UniqueVulkanShader solidColourShader;

		vk::UniqueDescriptorSet		descriptorSets[2];
	};
}