/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class TextureUploadExample : public VulkanTutorial	{
	public:
		TextureUploadExample(Window& window, VulkanInitialisation& vkInit);
		~TextureUploadExample() = default;
	protected:
		void RenderFrame(float dt) override;

		void UploadTexture(vk::CommandBuffer buffer);

		VulkanPipeline	pipeline;

		UniqueVulkanMesh 	mesh;
		UniqueVulkanTexture	texture;

		vk::UniqueDescriptorSet			descriptorSet;

		VulkanBuffer stagingBuffer;
	};
}