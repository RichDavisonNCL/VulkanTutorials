/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering::Vulkan {
	/*
	DynamicRenderBuilder: This helper class provides a means to create the 
	vkRenderingInfoKHR struct used by the dynamic rendering extension.

	Build returns one of these structs, and the class is designed to be used
	as an anonymous instantiation as the parameter to vkCmdBeginRenderingKHR

	BeginRendering will call vkCmdBeginRenderingKHR and can optionally also
	set up the viewport and scissor state to match the render area...
	*/
	class DynamicRenderBuilder	{
	public:
		DynamicRenderBuilder();
		~DynamicRenderBuilder() {}

		DynamicRenderBuilder& WithColourAttachment(
			vk::ImageView	texture,
			vk::ImageLayout m_layout = vk::ImageLayout::eColorAttachmentOptimal,
			bool clear = true,
			vk::ClearValue clearValue = vk::ClearColorValue(std::array<float, 4>{0, 0, 0, 1})
		);

		DynamicRenderBuilder& WithDepthAttachment(
			vk::ImageView	texture,
			vk::ImageLayout m_layout = vk::ImageLayout::eDepthAttachmentOptimal,
			bool clear = true,
			vk::ClearValue clearValue = vk::ClearDepthStencilValue(1.0f, 0),
			bool withStencil = false
		);

		DynamicRenderBuilder& WithColourAttachment(vk::RenderingAttachmentInfoKHR const & info);
		DynamicRenderBuilder& WithDepthAttachment(vk::RenderingAttachmentInfoKHR const& info);

		DynamicRenderBuilder& WithRenderArea(vk::Rect2D area, bool useAutoViewstate = true);
		DynamicRenderBuilder& WithRenderArea(vk::Offset2D offset, vk::Extent2D extent, bool useAutoViewstate = true);
		DynamicRenderBuilder& WithRenderArea(int32_t offsetX, int32_t offsetY, uint32_t extentX, uint32_t extentY, bool useAutoViewstate = true);

		DynamicRenderBuilder& WithRenderingFlags(vk::RenderingFlags flags);
		DynamicRenderBuilder& WithViewMask(uint32_t viewMask);
		DynamicRenderBuilder& WithLayerCount(int count);
		DynamicRenderBuilder& WithRenderInfo(vk::RenderingInfoKHR const & info);

		const vk::RenderingInfoKHR& Build();
		void BeginRendering(vk::CommandBuffer m_cmdBuffer);

	protected:
		vk::RenderingInfoKHR m_renderInfo;
		std::vector< vk::RenderingAttachmentInfoKHR > m_colourAttachments;
		vk::RenderingAttachmentInfoKHR m_depthAttachment;
		bool		m_usingStencil			= false;
		bool		m_usingAutoViewstate	= true;
	};
}