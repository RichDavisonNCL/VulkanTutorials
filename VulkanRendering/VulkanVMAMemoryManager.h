/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "VulkanMemoryManager.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

#include "vma/vk_mem_alloc.h"

namespace NCL::Rendering::Vulkan {
	struct	VulkanInitialisation;
	class	VMABuffer;

	class VulkanVMAMemoryManager : public VulkanMemoryManager {
	public:
		VulkanVMAMemoryManager(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance, const VulkanInitialisation& vkInit);
		virtual ~VulkanVMAMemoryManager();

		VulkanBuffer	CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags flags, const std::string& debugName = "")	override;
		VulkanBuffer	CreateStagingBuffer(size_t size, const std::string& debugName = "")						override;
		void			DiscardBuffer(VulkanBuffer& buffer, DiscardMode discard)								override;

		void*			MapBuffer(const VulkanBuffer& buffer)			override;
		void			UnmapBuffer(const VulkanBuffer& buffer)		override;
		void			CopyData(const VulkanBuffer& buffer, void* data, size_t size, size_t offset = 0) override;

		vk::Image		CreateImage(vk::ImageCreateInfo& createInfo, const std::string& debugName = "")		override;
		void			DiscardImage(vk::Image& tex, DiscardMode discard)									override;

		void			Update() override;

	protected:
		uint32_t	GetSpareBufferID();
		void		DeleteBuffer(VulkanBuffer& buffer);

		struct DeferredBufferDeletion {
			VulkanBuffer	buffer;
			uint32_t		framesCount;
		};

		struct Allocation {
			VmaAllocation			m_allocationHandle	= {};
			VmaAllocationInfo		m_allocationInfo	= {};
		};

		std::map<vk::Image, Allocation>		m_imageAllocations;

		std::vector<Allocation>				m_bufferAllocations;
		std::vector<uint32_t>				m_spareBufferIDs;

		std::vector<DeferredBufferDeletion> m_deferredDeleteBuffers;

		VmaAllocator			m_memoryAllocator;
		VmaAllocatorCreateInfo	m_allocatorInfo;

		uint32_t m_framesInFlight;
	};


}