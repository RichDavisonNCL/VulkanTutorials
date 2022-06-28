/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
namespace NCL::Rendering {
	class VulkanRenderer;
	class VulkanTexture;
	class VulkanRenderPassBuilder	{
	public:
		VulkanRenderPassBuilder(const std::string& name);
		~VulkanRenderPassBuilder() {}

		VulkanRenderPassBuilder& WithColourAttachment(
			VulkanTexture* texture, 
			bool clear	= true,
			vk::ImageLayout startLayout = vk::ImageLayout::eColorAttachmentOptimal, 
			vk::ImageLayout useLayout	= vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout endLayout	= vk::ImageLayout::eColorAttachmentOptimal
		);

		VulkanRenderPassBuilder& WithDepthAttachment(
			VulkanTexture* texture, 
			bool clear	= true,
			vk::ImageLayout startLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			vk::ImageLayout useLayout	= vk::ImageLayout::eDepthAttachmentOptimal,
			vk::ImageLayout endLayout	= vk::ImageLayout::eDepthAttachmentOptimal
		);

		VulkanRenderPassBuilder& WithDepthStencilAttachment(
			VulkanTexture* texture,
			bool clear = true,
			vk::ImageLayout startLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout useLayout	= vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout endLayout	= vk::ImageLayout::eDepthStencilAttachmentOptimal
		);

		vk::UniqueRenderPass Build(vk::Device renderer);

	protected:
		std::vector<vk::AttachmentDescription>	allDescriptions;
		std::vector<vk::AttachmentReference>		allReferences;
		vk::AttachmentReference depthReference;
		vk::SubpassDescription	subPass;
		std::string debugName;
	};
}