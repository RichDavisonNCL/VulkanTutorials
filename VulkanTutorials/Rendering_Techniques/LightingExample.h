/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class LightingExample : public VulkanTutorial {
	public:
		LightingExample(Window& window, VulkanInitialisation& vkInit);
		~LightingExample() {}

	protected:
		void RenderFrame(float dt) override;

		VulkanPipeline pipeline;

		UniqueVulkanShader	lightingShader;
		UniqueVulkanMesh	cubeMesh;

		vk::UniqueDescriptorSet			lightDescriptor;
		vk::UniqueDescriptorSet			cameraPosDescriptor;

		VulkanBuffer lightUniform;
		VulkanBuffer camPosUniform;

		RenderObject boxObject;
		RenderObject floorObject;

		UniqueVulkanTexture allTextures[4];
	};
}