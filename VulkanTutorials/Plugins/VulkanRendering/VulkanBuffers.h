/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering {
	struct VulkanBuffer {
		vk::UniqueBuffer			buffer;
		vk::MemoryAllocateInfo		allocInfo;
		vk::UniqueDeviceMemory		deviceMem;
		size_t						requestedSize;
	};

	struct VulkanAccelerationStructure : public VulkanBuffer {
		vk::AccelerationStructureKHR	structureInfo;

		VulkanAccelerationStructure() {

		}
		VulkanAccelerationStructure(VulkanBuffer&& b)
		{
			this->buffer		= std::move(b.buffer);
			this->allocInfo		= b.allocInfo;
			this->deviceMem		= std::move(b.deviceMem);
		}
	};
};