/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"
#include "../GLTFLoader/GLTFLoader.h"

namespace NCL::Rendering::Vulkan {
	class GLTFExample : public VulkanTutorial
	{
	public:
		GLTFExample(Window& window);
		~GLTFExample() {}

	protected:
		void RenderFrame(float dt) override;

		UniqueVulkanShader shader;

		GLTFScene  scene;

		std::vector<std::vector<vk::UniqueDescriptorSet>>	 layerDescriptors;

		std::vector < vk::UniqueDescriptorSet > layerSets;

		VulkanPipeline		pipeline;
	};
}

