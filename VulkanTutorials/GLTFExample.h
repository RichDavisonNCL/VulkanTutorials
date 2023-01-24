/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"
#include "../GLTFLoader/GLTFLoader.h"

namespace NCL::Rendering {
	class GLTFExample : public VulkanTutorialRenderer
	{
	public:
		GLTFExample(Window& window);
		~GLTFExample() {}

		void RenderFrame()		override;
	protected:
		UniqueVulkanShader shader;

		GLTFLoader loader;

		vk::UniqueDescriptorSetLayout	textureLayout;
		vector<vector<vk::UniqueDescriptorSet>>	 layerDescriptors;

		vector < vk::UniqueDescriptorSet > layerSets;

		VulkanPipeline		pipeline;
	};
}

