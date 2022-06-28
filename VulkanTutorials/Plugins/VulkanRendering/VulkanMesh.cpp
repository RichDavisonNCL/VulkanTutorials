/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanMesh.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

//These are both carefully arranged to match the MeshBuffer enum class!
vk::Format attributeFormats[] = {
	vk::Format::eR32G32B32Sfloat,	//Positions have this format
	vk::Format::eR32G32B32A32Sfloat,//Colours
	vk::Format::eR32G32Sfloat,		//TexCoords
	vk::Format::eR32G32B32Sfloat,	//Normals
	vk::Format::eR32G32B32A32Sfloat,//Tangents are 4D!
	vk::Format::eR32G32B32A32Sfloat,//Skel Weights
	vk::Format::eR32G32B32A32Sint,//Skel indices
};

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

VulkanMesh::VulkanMesh(const std::string& filename) : MeshGeometry(filename) {
}

VulkanMesh::~VulkanMesh()	{
}

void VulkanMesh::UploadToGPU(RendererBase* r)  {
	if (!ValidateMeshData()) {
		return;
	}
	VulkanRenderer* renderer = (VulkanRenderer*)r;

	sourceDevice = renderer->GetDevice();

	int vSize		= 0; //how much data each vertex takes up

	usedAttributes.clear();
	attributeBindings.clear();
	attributeDescriptions.clear();

	vector<const char*> attributeDataSources;

	auto atrributeFunc = [&](VertexAttribute attribute, size_t count, const char* data) {
		if (count > 0) {
			usedAttributes.emplace_back(attribute);
			attributeDataSources.push_back(data);
			vSize += (int)attributeSizes[attribute];
		}
	};

	atrributeFunc(VertexAttribute::Positions,		GetPositionData().size()	, (const char*)GetPositionData().data());
	atrributeFunc(VertexAttribute::Colours,			GetColourData().size()		, (const char*)GetColourData().data());
	atrributeFunc(VertexAttribute::TextureCoords,	GetTextureCoordData().size(), (const char*)GetTextureCoordData().data());
	atrributeFunc(VertexAttribute::Normals,			GetNormalData().size()		, (const char*)GetNormalData().data());
	atrributeFunc(VertexAttribute::Tangents,		GetTangentData().size()		, (const char*)GetTangentData().data());
	atrributeFunc(VertexAttribute::JointWeights,	GetSkinWeightData().size()	, (const char*)GetSkinWeightData().data());
	atrributeFunc(VertexAttribute::JointIndices,	GetSkinIndexData().size()	, (const char*)GetSkinIndexData().data());

	size_t vertexDataSize = vSize * GetVertexCount();

	vertexBuffer = renderer->CreateBuffer(vertexDataSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	VulkanBuffer stagingBuffer = renderer->CreateBuffer(vertexDataSize,
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible);

	//need to now copy vertex data to device memory
	char* dataPtr = (char*)sourceDevice.mapMemory(*stagingBuffer.deviceMem, 0, stagingBuffer.allocInfo.allocationSize);
	size_t offset = 0;
	for (size_t i = 0; i < usedAttributes.size(); ++i) {
		usedBuffers.push_back(*vertexBuffer.buffer);
		usedOffsets.push_back(offset);
		size_t copySize = GetVertexCount() * attributeSizes[usedAttributes[i]];
		memcpy(dataPtr + offset, attributeDataSources[i], copySize);
		offset += copySize;
	}
	sourceDevice.unmapMemory(*stagingBuffer.deviceMem);

	{//Now to transfer the vertex data from the staging buffer to the vertex buffer
		vk::BufferCopy copyRegion;
		copyRegion.size = vertexDataSize;
		vk::CommandBuffer cmdBuffer = renderer->BeginCmdBuffer();
		cmdBuffer.copyBuffer(*stagingBuffer.buffer, *vertexBuffer.buffer, copyRegion);
		renderer->SubmitCmdBufferWait(cmdBuffer);
	}

	for (uint32_t i = 0; i < usedAttributes.size(); ++i) {
		int attributeType = usedAttributes[i]; //Shaders are locked to specific ids due to the binding locations, this converts to that locked ID

		attributeDescriptions.emplace_back(attributeType, i, attributeFormats[attributeType], 0);
		attributeBindings.emplace_back(i, (unsigned int)attributeSizes[attributeType], vk::VertexInputRate::eVertex);
	}

	vertexInputState = vk::PipelineVertexInputStateCreateInfo({},
		(uint32_t)attributeBindings.size()		, & attributeBindings[0],	//How many buffers this mesh will be taking data from
		(uint32_t)attributeDescriptions.size()	, & attributeDescriptions[0] //how many attributes + attribute info from above
	);

	if (GetIndexCount() > 0) {	//Make the index buffer if there are any!
		size_t indexDataSize = sizeof(int) * GetIndexCount();
		indexBuffer = renderer->CreateBuffer(indexDataSize,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		VulkanBuffer stagingBuffer = renderer->CreateBuffer(indexDataSize,
			vk::BufferUsageFlagBits::eTransferSrc, 
			vk::MemoryPropertyFlagBits::eHostVisible);

		char* dataPtr = (char*)sourceDevice.mapMemory(stagingBuffer.deviceMem.get(), 0, stagingBuffer.allocInfo.allocationSize);
		memcpy(dataPtr, GetIndexData().data(), indexDataSize);
		sourceDevice.unmapMemory(*stagingBuffer.deviceMem);

		vk::BufferCopy copyRegion;
		copyRegion.size = indexDataSize;

		vk::CommandBuffer cmdBuffer = renderer->BeginCmdBuffer();
		cmdBuffer.copyBuffer(*stagingBuffer.buffer, *indexBuffer.buffer, copyRegion);
		renderer->SubmitCmdBufferWait(cmdBuffer);
	}
	if (!debugName.empty()) {
		Vulkan::SetDebugName(sourceDevice, vk::ObjectType::eBuffer, Vulkan::GetVulkanHandle(*vertexBuffer.buffer), debugName + " vertex attributes");
		if (GetIndexCount() > 0) {
			Vulkan::SetDebugName(sourceDevice, vk::ObjectType::eBuffer, Vulkan::GetVulkanHandle(*indexBuffer.buffer), debugName + " vertex indices");
		}
	}
}

void VulkanMesh::BindToCommandBuffer(vk::CommandBuffer  buffer) const {
	buffer.bindVertexBuffers(0, (unsigned int)usedBuffers.size(), &usedBuffers[0], &usedOffsets[0]);

	if (GetIndexCount() > 0) {
		buffer.bindIndexBuffer(*indexBuffer.buffer, 0, vk::IndexType::eUint32);
	}
}