/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class BasicMultiPipelineRenderer : public VulkanTutorialRenderer {
	public:
		BasicMultiPipelineRenderer(Window& window);
		~BasicMultiPipelineRenderer() {}

		void SetupTutorial() override;

		void RenderFrame() override;

	protected:
		void BuildColourPipeline();
		void BuildTexturePipeline();

		VulkanPipeline	solidColourPipeline;
		VulkanPipeline	texturedPipeline;
		UniqueVulkanMesh	triangleMesh;

		UniqueVulkanTexture	textures[2];

		UniqueVulkanShader texturingShader;
		UniqueVulkanShader solidColourShader;

		vk::UniqueDescriptorSet		textureDescriptorSetA;
		vk::UniqueDescriptorSet		textureDescriptorSetB;
	};
}