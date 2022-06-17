/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/MeshGeometry.h"
#include "VulkanBuffers.h"

namespace NCL::Rendering {
	class VulkanMesh : public MeshGeometry {
	public:
		friend class VulkanRenderer;
		VulkanMesh();
		VulkanMesh(const std::string& filename);
		~VulkanMesh();

		const vk::PipelineVertexInputStateCreateInfo& GetVertexInputState() const {
			return vertexInputState;
		}

		vk::PrimitiveTopology GetVulkanTopology() const {
			switch (primType) {
				case GeometryPrimitive::Points:			return vk::PrimitiveTopology::ePointList;
				case GeometryPrimitive::Lines:			return vk::PrimitiveTopology::eLineList;
				case GeometryPrimitive::Triangles:		return vk::PrimitiveTopology::eTriangleList;
				case GeometryPrimitive::TriangleFan:	return vk::PrimitiveTopology::eTriangleFan;
				case GeometryPrimitive::TriangleStrip:	return vk::PrimitiveTopology::eTriangleStrip;
				case GeometryPrimitive::Patches:		return vk::PrimitiveTopology::ePatchList;
			}
			return vk::PrimitiveTopology::eTriangleList;
		}

		void BindToCommandBuffer(vk::CommandBuffer  buffer) const;
		void UploadToGPU(RendererBase* renderer) override;

	protected:
		vk::PipelineVertexInputStateCreateInfo				vertexInputState;
		std::vector<vk::VertexInputAttributeDescription>	attributeDescriptions;
		std::vector<vk::VertexInputBindingDescription>		attributeBindings;		
	
		VulkanBuffer vertexBuffer;
		VulkanBuffer indexBuffer;

		vector<vk::Buffer>			usedBuffers;
		vector<vk::DeviceSize>		usedOffsets;
		vector< VertexAttribute >	usedAttributes;

		vk::Device			sourceDevice;
	};
}