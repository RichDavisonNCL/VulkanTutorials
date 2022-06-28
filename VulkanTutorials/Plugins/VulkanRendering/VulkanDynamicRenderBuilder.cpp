/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanDynamicRenderBuilder.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

VulkanDynamicRenderBuilder::VulkanDynamicRenderBuilder() {
	usingStencil	= false;
	layerCount		= 1;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::WithColourAttachment(
	vk::ImageView	texture, vk::ImageLayout layout, bool clear, vk::ClearValue clearValue
)
{
	vk::RenderingAttachmentInfoKHR colourAttachment;
	colourAttachment.setImageView(texture)
		.setImageLayout(layout)
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(clearValue);

	colourAttachments.push_back(colourAttachment);

	return *this;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::WithDepthAttachment(
	vk::ImageView	texture, vk::ImageLayout layout, bool clear, vk::ClearValue clearValue, bool withStencil
)
{
	depthAttachment.setImageView(texture)
		.setImageLayout(layout)
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(clearValue);

	usingStencil = withStencil;

	return *this;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::WithRenderArea(vk::Rect2D area) {
	renderArea = area;
	return *this;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::WithLayerCount(int count) {
	layerCount = count;
	return *this;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::WithSecondaryBuffers() {
	renderInfo.flags |= vk::RenderingFlagBits::eContentsSecondaryCommandBuffers;
	return *this;
}

VulkanDynamicRenderBuilder& VulkanDynamicRenderBuilder::Begin(vk::CommandBuffer  buffer) {
	renderInfo.setLayerCount(layerCount)
		.setRenderArea(renderArea)
		.setColorAttachments(colourAttachments)
		.setPDepthAttachment(&depthAttachment);

	if (usingStencil) {
		renderInfo.setPStencilAttachment(&depthAttachment);
	}

	buffer.beginRendering(renderInfo, *NCL::Rendering::Vulkan::dispatcher);
	return *this;
}