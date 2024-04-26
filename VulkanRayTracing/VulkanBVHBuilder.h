/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../VulkanRendering/VulkanRenderer.h"
#include "../VulkanRendering/VulkanBuffers.h"
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
		VulkanBVHBuilder& WithAllocator(VmaAllocator inAllocator);
		VulkanBVHBuilder& WithCommandQueue(vk::Queue inQueue);
		VulkanBVHBuilder& WithCommandPool(vk::CommandPool inPool);

		vk::UniqueAccelerationStructureKHR Build(vk::BuildAccelerationStructureFlagsKHR flags, const std::string& debugName = "");
	protected:

		void BuildBLAS(vk::Device device, VmaAllocator allocator, vk::BuildAccelerationStructureFlagsKHR flags);
		void BuildTLAS(vk::Device device, VmaAllocator allocator, vk::BuildAccelerationStructureFlagsKHR flags);

		vk::BuildAccelerationStructureFlagsKHR flags;

		std::map<VulkanMesh*, uint32_t> uniqueMeshes;

		std::vector<VulkanBVHEntry> entries;
		std::vector<VulkanMesh*>	meshes;
		std::vector<Matrix4>		transforms;
		std::vector< BLASEntry>		blasBuildInfo;

		vk::Queue		queue;
		vk::CommandPool pool;
		vk::Device		sourceDevice;
		VmaAllocator	sourceAllocator;

		vk::UniqueAccelerationStructureKHR	tlas;
		VulkanBuffer						tlasBuffer;
	};
}