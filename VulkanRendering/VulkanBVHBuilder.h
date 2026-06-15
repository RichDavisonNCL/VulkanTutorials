/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../VulkanRendering/VulkanRenderer.h"
#include "../VulkanRendering/VulkanBuffer.h"
#include "VulkanBVHBuilder.h"

namespace NCL::Rendering::Vulkan {
	struct VulkanBVHEntry {
		Matrix4		modelMat;
		uint32_t	meshID;
		uint32_t	hitID;
		uint32_t	mask;
	};
					
	struct BLASEntry {
		VulkanBuffer buffer;
		vk::AccelerationStructureBuildGeometryInfoKHR	buildInfo;
		vk::AccelerationStructureBuildSizesInfoKHR		sizeInfo;
		vk::UniqueAccelerationStructureKHR				accelStructure;

		std::vector<vk::AccelerationStructureBuildRangeInfoKHR>	ranges;
		std::vector<vk::AccelerationStructureGeometryKHR>		geometries;
		std::vector<uint32_t> maxPrims;
	};

	class VulkanBVHBuilder	{
	public:
		VulkanBVHBuilder();
		~VulkanBVHBuilder();

		VulkanBVHBuilder& WithObject(VulkanMesh* m, const Matrix4& transform, uint32_t mask = ~0, uint32_t hitID = 0);

		VulkanBVHBuilder& WithDevice(vk::Device inDevice);
		VulkanBVHBuilder& WithAllocator(VulkanMemoryManager& inAllocator);
		VulkanBVHBuilder& WithCommandQueue(vk::Queue inQueue);
		VulkanBVHBuilder& WithCommandPool(vk::CommandPool inPool);

		vk::UniqueAccelerationStructureKHR Build(vk::BuildAccelerationStructureFlagsKHR flags, const std::string& debugName = "");
	protected:

		void BuildBLAS(vk::BuildAccelerationStructureFlagsKHR flags);
		void BuildTLAS(vk::BuildAccelerationStructureFlagsKHR flags);

		vk::BuildAccelerationStructureFlagsKHR m_flags;

		std::map<VulkanMesh*, uint32_t> m_uniqueMeshes;

		std::vector<VulkanBVHEntry> m_entries;
		std::vector<VulkanMesh*>	m_meshes;
		std::vector<Matrix4>		m_transforms;
		std::vector< BLASEntry>		m_blasBuildInfo;

		vk::Queue		m_queue;
		vk::CommandPool m_pool;
		vk::Device		m_device;

		VulkanMemoryManager* m_memoryManager;

		vk::UniqueAccelerationStructureKHR	m_tlas;
		VulkanBuffer						m_tlasBuffer;
	};
}