/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanRenderPassBuilder.h"
#include "VulkanTexture.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

VulkanRenderPassBuilder::VulkanRenderPassBuilder(const std::string& name) {
	subPass.setPDepthStencilAttachment(nullptr);
	debugName = name;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::WithColourAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout,  vk::ImageLayout endLayout) {
	allDescriptions.emplace_back(
		vk::AttachmentDescription()
		.setInitialLayout(startLayout)
		.setFinalLayout(endLayout)
		.setFormat(texture->GetFormat())
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
	);
	allReferences.emplace_back(vk::AttachmentReference((uint32_t)allReferences.size(), useLayout));

	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::WithDepthAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout, vk::ImageLayout endLayout) {
	allDescriptions.emplace_back(
		vk::AttachmentDescription()
		.setInitialLayout(startLayout)
		.setFinalLayout(endLayout)
		.setFormat(texture->GetFormat())
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
	);
	depthReference			= vk::AttachmentReference((uint32_t)allReferences.size(), useLayout);
	subPass.setPDepthStencilAttachment(&depthReference);
	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::WithDepthStencilAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout, vk::ImageLayout endLayout) {
	return WithDepthAttachment(texture, clear, startLayout, useLayout, endLayout); //we just get different default parameters!
}

vk::UniqueRenderPass VulkanRenderPassBuilder::Build(vk::Device device) {
	subPass.setColorAttachmentCount((uint32_t)allReferences.size())
		.setPColorAttachments(allReferences.data())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount((uint32_t)allDescriptions.size())
		.setPAttachments(allDescriptions.data())
		.setSubpassCount(1)
		.setPSubpasses(&subPass);

	vk::UniqueRenderPass pass = device.createRenderPassUnique(renderPassInfo);

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eRenderPass, Vulkan::GetVulkanHandle(*pass), debugName);
	}

	return pass;
}