/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering {
	class VulkanDynamicRenderBuilder
	{
	public:
		VulkanDynamicRenderBuilder();
		~VulkanDynamicRenderBuilder();

		VulkanDynamicRenderBuilder& WithColourAttachment(
			vk::ImageView	texture,
			vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal,
			bool clear = true,
			vk::ClearValue clearValue = {}
		);

		VulkanDynamicRenderBuilder& WithDepthAttachment(
			vk::ImageView	texture,
			vk::ImageLayout layout = vk::ImageLayout::eDepthAttachmentOptimal,
			bool clear = true,
			vk::ClearValue clearValue = {},
			bool withStencil = false
		);

		VulkanDynamicRenderBuilder& WithRenderArea(vk::Rect2D area);

		VulkanDynamicRenderBuilder& WithLayerCount(int count);

		VulkanDynamicRenderBuilder& Begin(vk::CommandBuffer  buffer);
	protected:
		std::vector< vk::RenderingAttachmentInfoKHR > colourAttachments;
		vk::RenderingAttachmentInfoKHR depthAttachment;
		bool usingStencil;
		int layerCount;
		vk::Rect2D renderArea;
	};
}