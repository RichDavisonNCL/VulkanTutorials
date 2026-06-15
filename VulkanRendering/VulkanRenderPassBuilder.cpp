/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanRenderPassBuilder.h"
#include "VulkanTexture.h"
#include "Vulkanrenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

RenderPassBuilder::RenderPassBuilder(vk::Device m_device) {
	m_subPass.setPDepthStencilAttachment(nullptr);
	m_sourceDevice = m_device;
}

RenderPassBuilder& RenderPassBuilder::WithColourAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout,  vk::ImageLayout endLayout) {
	m_allDescriptions.emplace_back(
		vk::AttachmentDescription()
		.setInitialLayout(startLayout)
		.setFinalLayout(endLayout)
		.setFormat(texture->GetFormat())
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
	);
	m_allReferences.emplace_back(vk::AttachmentReference((uint32_t)m_allReferences.size(), useLayout));

	return *this;
}

RenderPassBuilder& RenderPassBuilder::WithDepthAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout, vk::ImageLayout endLayout) {
	m_allDescriptions.emplace_back(
		vk::AttachmentDescription()
		.setInitialLayout(startLayout)
		.setFinalLayout(endLayout)
		.setFormat(texture->GetFormat())
		.setLoadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
	);
	m_depthReference			= vk::AttachmentReference((uint32_t)m_allReferences.size(), useLayout);
	m_subPass.setPDepthStencilAttachment(&m_depthReference);
	return *this;
}

RenderPassBuilder& RenderPassBuilder::WithDepthStencilAttachment(VulkanTexture* texture, bool clear, vk::ImageLayout startLayout, vk::ImageLayout useLayout, vk::ImageLayout endLayout) {
	return WithDepthAttachment(texture, clear, startLayout, useLayout, endLayout); //we just get different default parameters!
}

vk::UniqueRenderPass RenderPassBuilder::Build(const std::string& debugName) {
	m_subPass.setColorAttachmentCount((uint32_t)m_allReferences.size())
		.setPColorAttachments(m_allReferences.data())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount((uint32_t)m_allDescriptions.size())
		.setPAttachments(m_allDescriptions.data())
		.setSubpassCount(1)
		.setPSubpasses(&m_subPass);

	vk::UniqueRenderPass pass = m_sourceDevice.createRenderPassUnique(renderPassInfo);

	if (!debugName.empty()) {
		SetDebugName(m_sourceDevice, *pass, debugName);
	}

	return std::move(pass);
}