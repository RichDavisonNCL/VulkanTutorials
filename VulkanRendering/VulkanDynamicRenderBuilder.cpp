/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanDynamicRenderBuilder.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

DynamicRenderBuilder::DynamicRenderBuilder() {
	m_renderInfo.setLayerCount(1);
}

DynamicRenderBuilder& DynamicRenderBuilder::WithColourAttachment(vk::RenderingAttachmentInfoKHR const&  info) {
	m_colourAttachments.push_back(info);
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithDepthAttachment(vk::RenderingAttachmentInfoKHR const&  info) {
	m_depthAttachment = info;
	//TODO check stencil state, maybe in Build...
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithColourAttachment(
	vk::ImageView	texture, vk::ImageLayout m_layout, bool clear, vk::ClearValue clearValue
)
{
	vk::RenderingAttachmentInfoKHR colourAttachment;
	colourAttachment.setImageView(texture)
		.setImageLayout(m_layout)
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(clearValue);

	m_colourAttachments.push_back(colourAttachment);

	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithDepthAttachment(
	vk::ImageView	texture, vk::ImageLayout m_layout, bool clear, vk::ClearValue clearValue, bool withStencil
)
{
	m_depthAttachment.setImageView(texture)
		.setImageLayout(m_layout)
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(clearValue);

	m_usingStencil = withStencil;

	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithRenderArea(vk::Rect2D area, bool useAutoViewstate) {
	m_renderInfo.setRenderArea(area);
	m_usingAutoViewstate = useAutoViewstate;
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithRenderArea(int32_t offsetX, int32_t offsetY, uint32_t extentX, uint32_t extentY, bool useAutoViewstate) {
	vk::Rect2D area = {
		.offset{offsetX, offsetY},
		.extent{extentX, extentY}
	};
	
	m_renderInfo.setRenderArea(area);
	m_usingAutoViewstate = useAutoViewstate;
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithRenderArea(vk::Offset2D offset, vk::Extent2D extent, bool useAutoViewstate) {
	vk::Rect2D area = { offset, extent };

	m_renderInfo.setRenderArea(area);
	m_usingAutoViewstate = useAutoViewstate;
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithLayerCount(int count) {
	m_renderInfo.setLayerCount(count);
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithRenderingFlags(vk::RenderingFlags flags) {
	m_renderInfo.setFlags(flags);
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithViewMask(uint32_t viewMask) {
	m_renderInfo.setViewMask(viewMask);
	return *this;
}

DynamicRenderBuilder& DynamicRenderBuilder::WithRenderInfo(vk::RenderingInfoKHR const& info) {
	m_renderInfo = info;
	return *this;
}

const vk::RenderingInfoKHR& DynamicRenderBuilder::Build() {
	m_renderInfo
		.setColorAttachments(m_colourAttachments)
		.setPDepthAttachment(&m_depthAttachment);

	if (m_usingStencil) {
		m_renderInfo.setPStencilAttachment(&m_depthAttachment);
	}
	return m_renderInfo;
}

void DynamicRenderBuilder::BeginRendering(vk::CommandBuffer m_cmdBuffer) {
	m_cmdBuffer.beginRendering(Build());
	if (m_usingAutoViewstate) {

		vk::Extent2D	extent		= m_renderInfo.renderArea.extent;
		vk::Viewport	viewport	= vk::Viewport(0.0f, (float)extent.height, (float)extent.width, -(float)extent.height, 0.0f, 1.0f);
		vk::Rect2D		scissor		= vk::Rect2D(vk::Offset2D(0, 0), extent);

		m_cmdBuffer.setViewport(0, 1, &viewport);
		m_cmdBuffer.setScissor( 0, 1, &scissor);
	}
}