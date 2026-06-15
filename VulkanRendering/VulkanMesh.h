/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/Mesh.h"
#include "SmartTypes.h"
#include "VulkanBuffer.h"

namespace NCL::Rendering::Vulkan {
	class VulkanMemoryManager;

	class VulkanMesh : public Mesh {
	public:
		friend class VulkanRenderer;
		VulkanMesh();
		~VulkanMesh();

		const vk::PipelineVertexInputStateCreateInfo& GetVertexInputState() const {
			return m_vertexInputState;
		}

		void BindToCommandBuffer(vk::CommandBuffer  buffer) const;

		void Draw(vk::CommandBuffer  to, int instanceCount = 1);
		void DrawLayer(unsigned int layer, vk::CommandBuffer  to, int instanceCount = 1);
		void DrawAllLayers(vk::CommandBuffer  to, int instanceCount = 1);

		void UploadToGPU(RendererBase* renderer) override;

		void UploadToGPU(vk::CommandBuffer cmdBuffer, VulkanMemoryManager* memManager, vk::BufferUsageFlags extraUses = {});

		uint32_t	GetAttributeMask() const;
		size_t		CalculateGPUAllocationSize() const;
		vk::PrimitiveTopology GetVulkanTopology() const;

		size_t GetVertexStride() const {
			return m_vertexStride;
		}

		bool GetIndexInformation(vk::Buffer& outBuffer, uint32_t& outOffset, uint32_t& outRange, vk::IndexType& outType);
		bool GetAttributeInformation(VertexAttribute::Type v, vk::Buffer& outBuffer, uint32_t& outOffset, uint32_t& outRange, vk::Format& outFormat) const;

	protected:
		vk::PipelineVertexInputStateCreateInfo				m_vertexInputState;

		vk::IndexType	m_indexType	= vk::IndexType::eNoneKHR;
		VulkanBuffer	m_gpuBuffer;

		size_t		m_vertexDataOffset	= 0;
		size_t		m_indexDataOffset	= 0;
		size_t		m_vertexStride		= 0;
		uint32_t	m_attributeMask		= 0;

		std::vector<vk::Buffer>					m_usedBuffers;
		std::vector<vk::DeviceSize>				m_usedOffsets;
		std::vector<vk::Format>					m_usedFormats;
		std::vector< VertexAttribute::Type >	m_usedAttributes;		
		
		std::vector<vk::VertexInputAttributeDescription>	m_attributeDescriptions;
		std::vector<vk::VertexInputBindingDescription>		m_attributeBindings;	
	};
}