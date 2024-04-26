/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanShaderBindingTableBuilder.h"
#include "VulkanBufferBuilder.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanShaderBindingTableBuilder::VulkanShaderBindingTableBuilder(const std::string& inDebugName) {
	debugName = debugName;
}

VulkanShaderBindingTableBuilder::~VulkanShaderBindingTableBuilder() {

}

VulkanShaderBindingTableBuilder& VulkanShaderBindingTableBuilder::WithProperties(vk::PhysicalDeviceRayTracingPipelinePropertiesKHR deviceProps) {
	properties = deviceProps;
	return *this;
}

VulkanShaderBindingTableBuilder& VulkanShaderBindingTableBuilder::WithPipeline(vk::Pipeline inPipe, const vk::RayTracingPipelineCreateInfoKHR& inPipeCreateInfo) {
	pipeline = inPipe;
	pipeCreateInfo = &inPipeCreateInfo;
	return *this;
}

VulkanShaderBindingTableBuilder& VulkanShaderBindingTableBuilder::WithLibrary(const vk::RayTracingPipelineCreateInfoKHR& createInfo) {
	libraries.push_back(&createInfo);
	return *this;
}

void VulkanShaderBindingTableBuilder::FillIndices(const vk::RayTracingPipelineCreateInfoKHR* fromInfo, int& offset) {
	for (int i = 0; i < fromInfo->groupCount; ++i) {
		if (fromInfo->pGroups[i].type == vk::RayTracingShaderGroupTypeKHR::eGeneral) {
			int shaderType = fromInfo->pGroups[i].generalShader;

			if (fromInfo->pStages[shaderType].stage == vk::ShaderStageFlagBits::eRaygenKHR) {
				handleIndices[BindingTableOrder::RayGen].push_back(fromInfo->pGroups[i].generalShader + offset);

			}
			else if (fromInfo->pStages[shaderType].stage == vk::ShaderStageFlagBits::eMissKHR) {
				handleIndices[BindingTableOrder::Miss].push_back(fromInfo->pGroups[i].generalShader + offset);
			}
			else if(fromInfo->pStages[shaderType].stage == vk::ShaderStageFlagBits::eCallableKHR) {
				handleIndices[BindingTableOrder::Call].push_back(fromInfo->pGroups[i].generalShader + offset);
			}
		}
		else { //Must be a hit group
			handleIndices[BindingTableOrder::Hit].push_back(fromInfo->pGroups[i].closestHitShader + offset);
		}
	}
	offset += fromInfo->groupCount;
}

int MakeMultipleOf(int input, int multiple) {
	int count = input / multiple;
	int r = input % multiple;
	
	if (r != 0) {
		count += 1;
	}

	return count * multiple;
}

ShaderBindingTable VulkanShaderBindingTableBuilder::Build(vk::Device device, VmaAllocator allocator) {
	assert(pipeCreateInfo);
	assert(pipeline);

	ShaderBindingTable table;

	uint32_t handleCount = 1;
	uint32_t handleSize = properties.shaderGroupHandleSize;
	uint32_t alignedHandleSize = MakeMultipleOf(handleSize, properties.shaderGroupHandleAlignment);

	int offset = 0;
	FillIndices(pipeCreateInfo, offset); //Fills the handleIndices vectors

	uint32_t numShaderGroups = pipeCreateInfo->groupCount;
	for (auto& i : libraries) {
		numShaderGroups += i->groupCount;
		FillIndices(i, offset);
	}

	uint32_t totalHandleSize = numShaderGroups * handleSize;
	std::vector<uint8_t> handles(totalHandleSize);
	auto result = device.getRayTracingShaderGroupHandlesKHR(pipeline, 0, numShaderGroups, totalHandleSize, handles.data());

	uint32_t totalAllocSize = 0;

	for (int i = 0; i < 4; ++i) {
		table.regions[i].size	= MakeMultipleOf(alignedHandleSize * handleIndices[i].size(), properties.shaderGroupBaseAlignment);
		table.regions[i].stride = alignedHandleSize;
		totalAllocSize += table.regions[i].size;
	}
	table.regions[0].stride = table.regions[0].size;

	table.tableBuffer = BufferBuilder(device, allocator)
		.WithBufferUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR)
		.WithDeviceAddress()
		.WithHostVisibility()
		.Build(totalAllocSize, debugName + " SBT Buffer");

	vk::DeviceAddress address = device.getBufferAddress({ .buffer = table.tableBuffer.buffer });

	char* bufData = (char*)table.tableBuffer.Map();
	int dataOffset = 0;
	for (int i = 0; i < 4; ++i) { //For each group type
		int dataOffsetStart = dataOffset;
		table.regions[i].deviceAddress = address + dataOffsetStart;
	
		for (int j = 0; j < handleIndices[i].size(); ++j) { //For entries in that group
			memcpy(bufData + dataOffset, handles.data() + (handleIndices[i][j] * handleSize), handleSize);
			dataOffset += alignedHandleSize;
		}
		dataOffset = dataOffsetStart + table.regions[i].size;
	}

	table.tableBuffer.Unmap();

	return table;
}