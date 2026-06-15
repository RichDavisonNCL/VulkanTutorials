/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanVMAMemoryManager.h"
#include "VulkanTexture.h"
#include "Vulkanrenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanVMAMemoryManager::VulkanVMAMemoryManager(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance, const VulkanInitialisation& vkInit) {
	VmaVulkanFunctions funcs = {};
	m_allocatorInfo = {};
	funcs.vkGetInstanceProcAddr = ::vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr;
	funcs.vkGetDeviceProcAddr	= ::vk::detail::defaultDispatchLoaderDynamic.vkGetDeviceProcAddr;

	m_allocatorInfo.physicalDevice		= physicalDevice;
	m_allocatorInfo.device				= device;
	m_allocatorInfo.instance			= instance;
	m_allocatorInfo.vulkanApiVersion	= VK_MAKE_API_VERSION(0, vkInit.majorVersion, vkInit.minorVersion, 0);

	m_allocatorInfo.flags |= vkInit.vmaFlags;

	m_framesInFlight = vkInit.framesInFlight;

	m_allocatorInfo.pVulkanFunctions = &funcs;
	vmaCreateAllocator(&m_allocatorInfo, &m_memoryAllocator);
}

VulkanVMAMemoryManager::~VulkanVMAMemoryManager() {
	for (std::vector<DeferredBufferDeletion>::iterator i = m_deferredDeleteBuffers.begin();
		i != m_deferredDeleteBuffers.end(); ++i )
	{
		DeleteBuffer(i->buffer);
	}

	vmaDestroyAllocator(m_memoryAllocator);
}
//VulkanBuffer VulkanVMAMemoryManager::CreateBuffer(const BufferCreationInfo& createInfo, const std::string& debugName) {
VulkanBuffer VulkanVMAMemoryManager::CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memProperties, const std::string& debugName) {
	VulkanBuffer newBuffer = AllocateBuffer();

	uint32_t id = GetSpareBufferID();
	newBuffer.SetAssetID(id);

	size_t allocSize = createInfo.size;
	
	newBuffer.size = allocSize;
		
	VmaAllocationCreateInfo alloCreateInfo = {};
	alloCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	
	alloCreateInfo.requiredFlags = (VkMemoryPropertyFlags)memProperties;
	
	if (memProperties & vk::MemoryPropertyFlagBits::eHostVisible) {
		alloCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
	}
	
	if (memProperties & vk::MemoryPropertyFlagBits::eHostCoherent) {
		alloCreateInfo.flags |= (VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	}

	Allocation allocation;
	
	vmaCreateBuffer(m_memoryAllocator, (VkBufferCreateInfo*)&createInfo, &alloCreateInfo, (VkBuffer*)&(newBuffer.buffer), &allocation.m_allocationHandle, &allocation.m_allocationInfo);
	
	if (createInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {

		vk::Device d = m_allocatorInfo.device;

		newBuffer.deviceAddress = d.getBufferAddress(
			{		
				.buffer = newBuffer.buffer
			}
		);
	}
	
	if (!debugName.empty()) {
		SetDebugName(m_allocatorInfo.device,newBuffer.buffer, debugName);
	}

	m_bufferAllocations[id] = allocation;
	
	return newBuffer;
}

VulkanBuffer VulkanVMAMemoryManager::CreateStagingBuffer(size_t size, const std::string& debugName) {
	//BufferCreationInfo createInfo = {
	//	.createInfo = {
	//		.size	= size,
	//		.usage	= vk::BufferUsageFlagBits::eTransferSrc
	//	},
	//	.memProperties	= vk::MemoryPropertyFlagBits::eHostVisible,
	//};

	return CreateBuffer(
		{
			.size = size,
			.usage = vk::BufferUsageFlagBits::eTransferSrc
		}
		,vk::MemoryPropertyFlagBits::eHostVisible, debugName);
}

void VulkanVMAMemoryManager::DiscardBuffer(VulkanBuffer& buffer, DiscardMode discard) {
	if (discard == DiscardMode::Deferred) {
		m_deferredDeleteBuffers.push_back({ std::move(buffer), m_framesInFlight });
	}
	else {
		DeleteBuffer(buffer);
	}
}


vk::Image VulkanVMAMemoryManager::CreateImage(vk::ImageCreateInfo& createInfo, const std::string& debugName) {
	vk::Image	image;
	Allocation	imageAlloc;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vmaCreateImage(m_memoryAllocator, (VkImageCreateInfo*)&createInfo, &vmaallocInfo, (VkImage*)&image, &imageAlloc.m_allocationHandle, &imageAlloc.m_allocationInfo);

	m_imageAllocations[image] = imageAlloc;

	return image;
}

void VulkanVMAMemoryManager::DiscardImage(vk::Image& image, DiscardMode discard)	{
	auto it = m_imageAllocations.find(image);
	if (it != m_imageAllocations.end()) {
		vmaDestroyImage(m_memoryAllocator, image, it->second.m_allocationHandle);
		image = VK_NULL_HANDLE;
	}
}

void	VulkanVMAMemoryManager::Update() {
	for (std::vector<DeferredBufferDeletion>::iterator i = m_deferredDeleteBuffers.begin();
		i != m_deferredDeleteBuffers.end(); )
	{
		(*i).framesCount--;

		if ((*i).framesCount == 0) {
			DeleteBuffer(i->buffer);
			i = m_deferredDeleteBuffers.erase(i);
		}
		else {
			++i;
		}
	}
}

void* VulkanVMAMemoryManager::MapBuffer(const VulkanBuffer& buffer) {
	uint32_t id				= buffer.GetAssetID();
	Allocation allocation	= m_bufferAllocations[id];

	if (allocation.m_allocationInfo.pMappedData) {
		return allocation.m_allocationInfo.pMappedData;
	}
	void* mappedData = nullptr;
	vmaMapMemory(m_memoryAllocator, allocation.m_allocationHandle, &mappedData);
	return mappedData;

	return nullptr;
}

void	VulkanVMAMemoryManager::UnmapBuffer(const VulkanBuffer& buffer) {
	uint32_t id = buffer.GetAssetID();
	Allocation allocation = m_bufferAllocations[id];
	if (allocation.m_allocationInfo.pMappedData) {
		return;
	}
	vmaUnmapMemory(m_memoryAllocator, allocation.m_allocationHandle);
}

void	VulkanVMAMemoryManager::CopyData(const VulkanBuffer& buffer, void* data, size_t size, size_t offset) {
	uint32_t id = buffer.GetAssetID();
	Allocation allocation = m_bufferAllocations[id];

	if (allocation.m_allocationInfo.pMappedData) {
		//we're already mapped, can just copy
		memcpy((void*)((size_t)allocation.m_allocationInfo.pMappedData + offset), data, size);
	}
	else {
		//We should be able to safely map this?
		void* mappedData = nullptr;
		vmaMapMemory(m_memoryAllocator, allocation.m_allocationHandle, &mappedData);
		memcpy((void*)((size_t)mappedData + offset), data, size);
		vmaUnmapMemory(m_memoryAllocator, allocation.m_allocationHandle);
	}
}

uint32_t	VulkanVMAMemoryManager::GetSpareBufferID() {
	uint32_t id = 0;
	if (m_spareBufferIDs.empty()) {
		id = m_bufferAllocations.size();
		m_bufferAllocations.resize(m_bufferAllocations.size() + 1);
		return id;
	}
	else {
		id = m_spareBufferIDs.back();
		m_spareBufferIDs.pop_back();
	}
	return id;
}

void	VulkanVMAMemoryManager::DeleteBuffer(VulkanBuffer& buffer) {
	uint32_t id				= buffer.GetAssetID();

	if (!m_bufferAllocations[id].m_allocationHandle) {
		return;
	}

	vmaDestroyBuffer(m_memoryAllocator, buffer.buffer, m_bufferAllocations[id].m_allocationHandle);
	m_spareBufferIDs.push_back(id);

	m_bufferAllocations[id].m_allocationHandle = 0;
	m_bufferAllocations[id].m_allocationInfo = {};

	buffer.buffer			= VK_NULL_HANDLE;
	buffer.size				= 0;
	buffer.deviceAddress	= 0;
}