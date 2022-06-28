/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;

vk::DispatchLoaderDynamic* NCL::Rendering::Vulkan::dispatcher = nullptr;

void Vulkan::SetDebugName(vk::Device device, vk::ObjectType t, uint64_t handle, const std::string& debugName) {
	device.setDebugUtilsObjectNameEXT(
		vk::DebugUtilsObjectNameInfoEXT()
		.setObjectType(t)
		.setObjectHandle(handle)
		.setPObjectName(debugName.c_str()), *Vulkan::dispatcher
	);
};

void Vulkan::BeginDebugArea(vk::CommandBuffer b, const std::string& name) {
	vk::DebugUtilsLabelEXT labelInfo;
	labelInfo.pLabelName = name.c_str();

	b.beginDebugUtilsLabelEXT(labelInfo, *Vulkan::dispatcher);
}

void Vulkan::EndDebugArea(vk::CommandBuffer b) {
	b.endDebugUtilsLabelEXT(*Vulkan::dispatcher);
}