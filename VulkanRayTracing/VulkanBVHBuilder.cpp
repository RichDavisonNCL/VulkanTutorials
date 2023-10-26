/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanBVHBuilder.h"

#include "../VulkanRendering/VulkanMesh.h"
#include "../VulkanRendering/VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanBVHBuilder::VulkanBVHBuilder() {
}

VulkanBVHBuilder::~VulkanBVHBuilder() {
}

VulkanBVHBuilder& VulkanBVHBuilder::WithObject(VulkanMesh* m, const Matrix4& transform, uint32_t mask, uint32_t hitID) {
	auto savedMesh = uniqueMeshes.find(m);

	uint32_t meshID = 0;

	if (savedMesh == uniqueMeshes.end()) {
		meshes.push_back(m);
		meshID = meshes.size() - 1;
		uniqueMeshes.insert({m, meshID});
	}
	else {
		meshID = savedMesh->second;
	}

	VulkanBVHEntry entry;
	entry.modelMat	= transform;
	entry.meshID	= meshID;
	entry.hitID		= hitID;
	entry.mask		= mask;

	entries.push_back(entry);

	return *this;
}

VulkanBVHBuilder& VulkanBVHBuilder::WithCommandQueue(vk::Queue inQueue) {
	queue = inQueue;
	return *this;
}

VulkanBVHBuilder& VulkanBVHBuilder::WithCommandPool(vk::CommandPool inPool) {
	pool = inPool;
	return *this;
}

VulkanBVHBuilder& VulkanBVHBuilder::WithDevice(vk::Device inDevice) {
	sourceDevice = inDevice;
	return *this;
}

VulkanBVHBuilder& VulkanBVHBuilder::WithAllocator(VmaAllocator inAllocator) {
	sourceAllocator = inAllocator;
	return *this;
}

vk::UniqueAccelerationStructureKHR VulkanBVHBuilder::Build(vk::BuildAccelerationStructureFlagsKHR inFlags, const std::string& debugName) {
	BuildBLAS(sourceDevice, sourceAllocator, inFlags);
	BuildTLAS(sourceDevice, sourceAllocator, inFlags);

	if (!debugName.empty()) {
		SetDebugName(sourceDevice, vk::ObjectType::eAccelerationStructureKHR, GetVulkanHandle(*tlas), debugName);
	}

	return std::move(tlas);
}

void VulkanBVHBuilder::BuildBLAS(vk::Device device, VmaAllocator allocator, vk::BuildAccelerationStructureFlagsKHR inFlags) {
	//We need to first create the BLAS entries for the unique meshes
	for (const auto& i : uniqueMeshes) {
		vk::Buffer	vBuffer;
		uint32_t	vOffset;
		uint32_t	vRange;
		vk::Format	vFormat;
		i.first->GetAttributeInformation(NCL::VertexAttribute::Positions, vBuffer, vOffset, vRange, vFormat);

		vk::Buffer		iBuffer;
		uint32_t		iOffset;
		uint32_t		iRange;
		vk::IndexType	iFormat;
		bool hasIndices = i.first->GetIndexInformation(iBuffer, iOffset, iRange, iFormat);

		vk::AccelerationStructureGeometryTrianglesDataKHR triData;
		triData.vertexFormat = vFormat;
		triData.vertexData.deviceAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo(vBuffer)) + vOffset;
		triData.vertexStride = sizeof(Vector3);

		if (hasIndices) {		
			triData.indexType = iFormat;
			triData.indexData.deviceAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo(iBuffer)) + iOffset;		
		}

		triData.maxVertex = i.first->GetVertexCount();

		blasBuildInfo.resize(blasBuildInfo.size() + 1);

		BLASEntry& blasEntry = blasBuildInfo.back();

		size_t subMeshCount = i.first->GetSubMeshCount();

		blasEntry.buildInfo.geometryCount	= subMeshCount;

		blasEntry.geometries.resize(subMeshCount);
		blasEntry.ranges.resize(subMeshCount);
		blasEntry.maxPrims.resize(subMeshCount);

		for (int j = 0; j < subMeshCount; ++j) {
			const SubMesh* m = i.first->GetSubMesh(j);

			blasEntry.geometries[j].setGeometryType(vk::GeometryTypeKHR::eTriangles)
												.setFlags(vk::GeometryFlagBitsKHR::eOpaque)
												.geometry.setTriangles(triData);

			blasEntry.geometries[j].geometry.triangles.maxVertex = m->count;

			blasEntry.ranges[j].primitiveCount	= i.first->GetPrimitiveCount(j);
			blasEntry.ranges[j].firstVertex		= m->base;
			blasEntry.ranges[j].primitiveOffset = m->start *(iFormat == vk::IndexType::eUint32 ? 4 : 2);
			blasEntry.maxPrims[j] = i.first->GetPrimitiveCount(j); 
		}
	}

	vk::DeviceSize totalSize	= 0;
	vk::DeviceSize scratchSize	= 0;

	for (auto& i : blasBuildInfo) {	//Go through each of the added entries to build up data...
		i.buildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		i.buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		i.buildInfo.geometryCount	= i.geometries.size(); //TODO
		i.buildInfo.pGeometries		= i.geometries.data();
		i.buildInfo.flags |= inFlags;

		device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
			&i.buildInfo, i.maxPrims.data(), &i.sizeInfo);

		totalSize	+= i.sizeInfo.accelerationStructureSize;
		scratchSize  = std::max(scratchSize, i.sizeInfo.buildScratchSize);
	}

	VulkanBuffer scratchBuff = BufferBuilder(device, allocator)
		.WithBufferUsage(	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
							vk::BufferUsageFlagBits::eStorageBuffer)
		.Build(scratchSize, "Scratch Buffer");

	vk::DeviceAddress scratchAddr = device.getBufferAddress(scratchBuff.buffer);

	vk::UniqueCommandBuffer buffer = CmdBufferBegin(device, pool, "Making BLAS");

	for (auto& i : blasBuildInfo) {		//Make the buffer for each blas entry...
		vk::AccelerationStructureCreateInfoKHR createInfo;
		createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		createInfo.size = i.sizeInfo.accelerationStructureSize;

		i.buffer = BufferBuilder(device, allocator)
			.WithBufferUsage(	vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | 
								vk::BufferUsageFlagBits::eShaderDeviceAddress)
			.WithHostVisibility()
			.Build(createInfo.size);

		createInfo.buffer = i.buffer;

		i.accelStructure = device.createAccelerationStructureKHRUnique(createInfo);

		i.buildInfo.dstAccelerationStructure	= *i.accelStructure;
		i.buildInfo.scratchData.deviceAddress	= scratchAddr;

		const vk::AccelerationStructureBuildRangeInfoKHR* rangeInfo = i.ranges.data();

		buffer->buildAccelerationStructuresKHR(1, &i.buildInfo, &rangeInfo);
					
		buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
			vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, 
			{}, 
			vk::MemoryBarrier(	vk::AccessFlagBits::eAccelerationStructureWriteKHR,
								vk::AccessFlagBits::eAccelerationStructureReadKHR),
			{}, 
			{}
		);
	}
	CmdBufferEndSubmitWait(*buffer, device, queue);
}

void VulkanBVHBuilder::BuildTLAS(vk::Device device, VmaAllocator allocator, vk::BuildAccelerationStructureFlagsKHR flags) {
	std::vector<vk::AccelerationStructureInstanceKHR> tlasEntries;

	const uint32_t instanceCount = entries.size();

	tlasEntries.resize(instanceCount);

	for (int i = 0; i < instanceCount; ++i) {
		tlasEntries[i].transform.matrix[0][0] = entries[i].modelMat.array[0][0];
		tlasEntries[i].transform.matrix[0][1] = entries[i].modelMat.array[1][0];
		tlasEntries[i].transform.matrix[0][2] = entries[i].modelMat.array[2][0];
		tlasEntries[i].transform.matrix[0][3] = entries[i].modelMat.array[3][0];

		tlasEntries[i].transform.matrix[1][0] = entries[i].modelMat.array[0][1];
		tlasEntries[i].transform.matrix[1][1] = entries[i].modelMat.array[1][1];
		tlasEntries[i].transform.matrix[1][2] = entries[i].modelMat.array[2][1];
		tlasEntries[i].transform.matrix[1][3] = entries[i].modelMat.array[3][1];

		tlasEntries[i].transform.matrix[2][0] = entries[i].modelMat.array[0][2];
		tlasEntries[i].transform.matrix[2][1] = entries[i].modelMat.array[1][2];
		tlasEntries[i].transform.matrix[2][2] = entries[i].modelMat.array[2][2];
		tlasEntries[i].transform.matrix[2][3] = entries[i].modelMat.array[3][2];

		uint32_t meshID = entries[i].meshID;

		tlasEntries[i].instanceCustomIndex = meshID;

		tlasEntries[i].accelerationStructureReference = device.getBufferAddress(blasBuildInfo[meshID].buffer.buffer);
		tlasEntries[i].flags = (VkGeometryInstanceFlagBitsKHR)vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable;
		tlasEntries[i].mask = entries[i].mask;
		tlasEntries[i].instanceShaderBindingTableRecordOffset = entries[i].hitID;
	}

	size_t dataSize = instanceCount * sizeof(vk::AccelerationStructureInstanceKHR);

	VulkanBuffer instanceBuffer = BufferBuilder(device, allocator)
		.WithBufferUsage(	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
							vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
		.WithHostVisibility()
		.Build(dataSize, "Instance Buffer");

	instanceBuffer.CopyData(tlasEntries.data(), dataSize);

	vk::AccelerationStructureGeometryKHR tlasGeometry;
	tlasGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
	tlasGeometry.geometry = vk::AccelerationStructureGeometryInstancesDataKHR();
	tlasGeometry.geometry.instances.data = device.getBufferAddress(instanceBuffer.buffer);

	vk::AccelerationStructureBuildGeometryInfoKHR geomInfo;
	geomInfo.flags			= flags;
	geomInfo.geometryCount	= 1;
	geomInfo.pGeometries	= &tlasGeometry;
	geomInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	geomInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	geomInfo.srcAccelerationStructure = nullptr; //??

	vk::AccelerationStructureBuildSizesInfoKHR sizesInfo;
	device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &geomInfo, &instanceCount, &sizesInfo);

	tlasBuffer = BufferBuilder(device, allocator)
		.WithDeviceAddress()
		.WithBufferUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR)
		.Build(sizesInfo.accelerationStructureSize, "TLAS Buffer");

	vk::AccelerationStructureCreateInfoKHR tlasCreateInfo;
	tlasCreateInfo.buffer = tlasBuffer.buffer;
	tlasCreateInfo.size = sizesInfo.accelerationStructureSize;
	tlasCreateInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	
	tlas = device.createAccelerationStructureKHRUnique(tlasCreateInfo);

	VulkanBuffer scratchBuffer = BufferBuilder(device, allocator)
		.WithBufferUsage(	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
							vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.WithDeviceAddress()
		.Build(sizesInfo.buildScratchSize, "Scratch Buffer");

	vk::DeviceAddress scratchAddr = device.getBufferAddress(scratchBuffer.buffer);

	geomInfo.srcAccelerationStructure = nullptr;
	geomInfo.dstAccelerationStructure = *tlas;
	geomInfo.scratchData.deviceAddress = scratchAddr;

	vk::AccelerationStructureBuildRangeInfoKHR rangeInfo;
	rangeInfo.primitiveCount = instanceCount;

	vk::AccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &rangeInfo;

	vk::UniqueCommandBuffer cmdBuffer = CmdBufferBegin(device, pool, "Making TLAS");
	cmdBuffer->buildAccelerationStructuresKHR(1, &geomInfo, &rangeInfoPtr);
	CmdBufferEndSubmitWait(*cmdBuffer, device, queue);
}