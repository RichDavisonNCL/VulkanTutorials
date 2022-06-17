/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanRenderer.h"
#include <unordered_set>
#include <vector>

namespace NCL::Maths {
	class Matrix4;
}

namespace NCL::Rendering {
	class VulkanBVH {
		vk::AccelerationStructureGeometryKHR		geometry;
		vk::AccelerationStructureBuildRangeInfoKHR	info;
		//TODO
	};

	class VulkanBVHBuilder	{
	public:
		VulkanBVHBuilder();
		~VulkanBVHBuilder();

		VulkanBVHBuilder& WithMesh(VulkanMesh* m, const Matrix4& transform);//how to set binding table???

		VulkanBVH Build(VulkanRenderer& renderer);

	protected:
		std::unordered_set<VulkanMesh*> uniqueMeshes;
		std::vector<VulkanMesh*> meshes;
		std::vector<Matrix4>		transforms;

		unsigned int vertexCount;
		unsigned int indexCount;
	};
}