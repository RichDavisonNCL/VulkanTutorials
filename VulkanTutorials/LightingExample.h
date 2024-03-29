/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class LightingExample : public VulkanTutorialRenderer {
	public:
		LightingExample(Window& window);
		~LightingExample() {}

		void SetupTutorial() override;

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		void	BuildPipeline();

		VulkanPipeline pipeline;

		UniqueVulkanShader	lightingShader;
		UniqueVulkanMesh	cubeMesh;

		vk::UniqueDescriptorSet			lightDescriptor;
		vk::UniqueDescriptorSetLayout	lightLayout;

		vk::UniqueDescriptorSet			cameraPosDescriptor;
		vk::UniqueDescriptorSetLayout	cameraPosLayout;

		vk::UniqueDescriptorSetLayout	texturesLayout;

		VulkanBuffer lightUniform;
		VulkanBuffer camPosUniform;

		RenderObject boxObject;
		RenderObject floorObject;

		UniqueVulkanTexture allTextures[4];
	};
}