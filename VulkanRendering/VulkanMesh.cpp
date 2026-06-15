/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanMesh.h"
#include "Vulkanrenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

//These are both carefully arranged to match the MeshBuffer enum class!
vk::Format attributeFormats[] = { 
	vk::Format::eR32G32B32Sfloat,	//Positions have this format
	vk::Format::eR32G32B32A32Sfloat,//Colours
	vk::Format::eR32G32Sfloat,		//TexCoords
	vk::Format::eR32G32B32Sfloat,	//Normals
	vk::Format::eR32G32B32A32Sfloat,//Tangents are 4D!
	vk::Format::eR32G32B32A32Sfloat,//Skel Weights
	vk::Format::eR32G32B32A32Sint,	//Skel indices
};
//Attribute sizes for each of the above
size_t attributeSizes[] = {
	sizeof(Vector3),
	sizeof(Vector4),
	sizeof(Vector2),
	sizeof(Vector3),
	sizeof(Vector4),
	sizeof(Vector4),
	sizeof(Vector4),
};

VulkanMesh::VulkanMesh()	{

}

VulkanMesh::~VulkanMesh()	{

}

void VulkanMesh::UploadToGPU(vk::CommandBuffer cmdBuffer, VulkanMemoryManager* memManager, vk::BufferUsageFlags extraUses) {
	assert(ValidateMeshData());
	
	size_t allocationSize = CalculateGPUAllocationSize();
	
	m_indexType = GetIndexCount() > 0 ? vk::IndexType::eUint32 : vk::IndexType::eNoneKHR;
	
	m_usedAttributes.clear();
	m_attributeBindings.clear();
	m_attributeDescriptions.clear();
	
	std::vector<const char*> attributeDataSources;//Pointer for each attribute in CPU memory
	
	size_t vSize = 0;
	
	auto atrributeFunc = [&](VertexAttribute::Type attribute, size_t count, const char* data) {
		if (count > 0) {
			m_usedAttributes.push_back(attribute);
			m_usedFormats.push_back(attributeFormats[attribute]);
			attributeDataSources.push_back(data);
			vSize += attributeSizes[attribute];
		}
		};
	
	atrributeFunc(VertexAttribute::Positions, GetPositionData().size(), (const char*)GetPositionData().data());
	atrributeFunc(VertexAttribute::Colours, GetColourData().size(), (const char*)GetColourData().data());
	atrributeFunc(VertexAttribute::TextureCoords, GetTextureCoordData().size(), (const char*)GetTextureCoordData().data());
	atrributeFunc(VertexAttribute::Normals, GetNormalData().size(), (const char*)GetNormalData().data());
	atrributeFunc(VertexAttribute::Tangents, GetTangentData().size(), (const char*)GetTangentData().data());
	atrributeFunc(VertexAttribute::JointWeights, GetSkinWeightData().size(), (const char*)GetSkinWeightData().data());
	atrributeFunc(VertexAttribute::JointIndices, GetSkinIndexData().size(), (const char*)GetSkinIndexData().data());
	
	for (uint32_t i = 0; i < m_usedAttributes.size(); ++i) {
		//Which vertex attribute slot should Vulkan buffer index i map to?
		int attributeType = m_usedAttributes[i];
		//Describes the vertex attribute state
		m_attributeBindings.emplace_back(i, (unsigned int)attributeSizes[attributeType], vk::VertexInputRate::eVertex);
		//Describes the vertex attribute data type and offset
		m_attributeDescriptions.emplace_back(attributeType, i, attributeFormats[attributeType], 0);
	
		m_attributeMask |= (1 << attributeType);
	}
	
	m_vertexInputState = vk::PipelineVertexInputStateCreateInfo(
		{
			.flags = {},
			.vertexBindingDescriptionCount		= (uint32_t)m_attributeBindings.size(),
			.pVertexBindingDescriptions			= &m_attributeBindings[0],
			.vertexAttributeDescriptionCount	= (uint32_t)m_attributeDescriptions.size(),
			.pVertexAttributeDescriptions		= &m_attributeDescriptions[0]
		}
	);
	
	size_t vertexDataSize		= vSize * GetVertexCount();
	size_t indexDataSize		= sizeof(int) * GetIndexCount();
	size_t totalAllocationSize	= vertexDataSize + indexDataSize;

	VulkanBuffer stagingBuffer = memManager->CreateStagingBuffer(totalAllocationSize);

	m_gpuBuffer = memManager->CreateBuffer(
		{
			.size	= totalAllocationSize,
			.usage	=   vk::BufferUsageFlagBits::eIndexBuffer	|
						vk::BufferUsageFlagBits::eVertexBuffer	|
						vk::BufferUsageFlagBits::eTransferDst	|
						extraUses
		},
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		"Vertex / Index Buffer"
	);

	assert(stagingBuffer.size >= (totalAllocationSize));
	
	//need to now copy vertex data to device memory
	char* dataPtr = (char*)stagingBuffer.Map();
	size_t offset = 0;
	for (size_t i = 0; i < m_usedAttributes.size(); ++i) {
		//We're going to use the same buffer for every attribute
		m_usedBuffers.push_back(m_gpuBuffer.buffer);
		//But each attribute starts at a different offset
		m_usedOffsets.push_back(offset);
		//Copy the data from CPU to GPU-visible memory
		size_t copySize = GetVertexCount() * attributeSizes[m_usedAttributes[i]];
		memcpy(dataPtr + offset, attributeDataSources[i], copySize);
		offset += copySize;
	}
	
	if (GetIndexCount() > 0) {
		memcpy(dataPtr + offset, GetIndexData().data(), indexDataSize);
		m_indexType = vk::IndexType::eUint32;
		m_indexDataOffset = offset;
	}
	stagingBuffer.Unmap();
	
	{//Now to transfer the mesh data from the staging buffer to the gpu-only buffer
		vk::BufferCopy copyRegion;
		copyRegion.size = vertexDataSize + indexDataSize;
		cmdBuffer.copyBuffer(stagingBuffer.buffer, m_gpuBuffer.buffer, copyRegion);
	}
	
	//Shove in a semaphore
	memManager->DiscardBuffer(stagingBuffer);
}

void VulkanMesh::UploadToGPU(RendererBase* r)  {
//	UploadToGPU(r, {});
}

void VulkanMesh::BindToCommandBuffer(vk::CommandBuffer  buffer) const {
	buffer.bindVertexBuffers(0, m_usedBuffers.size(), &m_usedBuffers[0], &m_usedOffsets[0]);

	if (GetIndexCount() > 0) {
		buffer.bindIndexBuffer(m_gpuBuffer.buffer, m_indexDataOffset, m_indexType);
	}
}

void VulkanMesh::DrawLayer(unsigned int layer, vk::CommandBuffer  to, int instanceCount) {
	const SubMesh* sm = GetSubMesh(layer);

	BindToCommandBuffer(to);

	if (GetIndexCount() > 0) {
		to.drawIndexed(sm->count, instanceCount, sm->start, sm->base, 0);
	}
	else {
		to.draw(sm->count, instanceCount, sm->start, 0);
	}
}

void VulkanMesh::Draw(vk::CommandBuffer  to, int instanceCount) {
	BindToCommandBuffer(to);

	if (GetIndexCount() > 0) {
		to.drawIndexed(GetIndexCount(), instanceCount, 0, 0, 0);
	}
	else {
		to.draw(GetVertexCount(), instanceCount, 0, 0);
	}
}

void VulkanMesh::DrawAllLayers(vk::CommandBuffer  to, int instanceCount) {
	BindToCommandBuffer(to);
	if (GetIndexCount() > 0) {
		for (int i = 0; i < subMeshes.size(); ++i) {
			const SubMesh* sm = GetSubMesh(i);
			to.drawIndexed(sm->count, instanceCount, sm->start, sm->base, 0);
		}
	}
	else {
		for (int i = 0; i < subMeshes.size(); ++i) {
			const SubMesh* sm = GetSubMesh(i);
			to.draw(sm->count, instanceCount, sm->start, 0);
		}
	}

}


vk::PrimitiveTopology VulkanMesh::GetVulkanTopology() const {
	assert((uint32_t)primType < GeometryPrimitive::MAX_PRIM);

	const vk::PrimitiveTopology table[] = {
		vk::PrimitiveTopology::ePointList,
		vk::PrimitiveTopology::eLineList,
		vk::PrimitiveTopology::eTriangleList,
		vk::PrimitiveTopology::eTriangleFan,
		vk::PrimitiveTopology::eTriangleStrip,
		vk::PrimitiveTopology::ePatchList
	};
	return table[(uint32_t)primType];
}

uint32_t VulkanMesh::GetAttributeMask() const {
	return m_attributeMask;
}

size_t VulkanMesh::CalculateGPUAllocationSize() const {
	size_t vSize = 0;
	auto atrributeSizeFunc = [&](VertexAttribute::Type attribute, size_t count) {
		if (count > 0) {
			vSize += (int)attributeSizes[attribute];
		}
	};

	atrributeSizeFunc(VertexAttribute::Positions, GetPositionData().size());
	atrributeSizeFunc(VertexAttribute::Colours, GetColourData().size());
	atrributeSizeFunc(VertexAttribute::TextureCoords, GetTextureCoordData().size());
	atrributeSizeFunc(VertexAttribute::Normals, GetNormalData().size());
	atrributeSizeFunc(VertexAttribute::Tangents, GetTangentData().size());
	atrributeSizeFunc(VertexAttribute::JointWeights, GetSkinWeightData().size());
	atrributeSizeFunc(VertexAttribute::JointIndices, GetSkinIndexData().size());

	size_t vertexDataSize = vSize * GetVertexCount();
	size_t indexDataSize = 0;

	if (GetIndexCount() > 0) {
		int elementSize = (m_indexType == vk::IndexType::eUint32) ? 4 : 2;
		indexDataSize = elementSize * GetIndexCount();
	}
	return vertexDataSize + indexDataSize;
}

bool VulkanMesh::GetIndexInformation(vk::Buffer& outBuffer, uint32_t& outOffset, uint32_t& outRange, vk::IndexType& outType) {
	if (m_indexType == vk::IndexType::eNoneKHR) {
		return false;
	}

	int elementSize = (m_indexType == vk::IndexType::eUint32) ? 4: 2;
	
	outBuffer	= m_gpuBuffer.buffer;
	outOffset	= m_indexDataOffset;
	outRange	= elementSize * GetIndexCount();
	outType		= m_indexType;

	return true;
}

bool VulkanMesh::GetAttributeInformation(VertexAttribute::Type v, vk::Buffer& outBuffer, uint32_t& outOffset, uint32_t& outRange, vk::Format& outFormat) const {
	for (uint32_t i = 0; i < m_usedAttributes.size(); ++i) {
		if (m_usedAttributes[i] != v) {
			continue;
		}

		outBuffer	= m_usedBuffers[i];
		outOffset	= m_usedOffsets[i];
		outRange	= attributeSizes[v] * GetVertexCount();
		outFormat	= m_usedFormats[i];

		return true;
	}

	return false;
}