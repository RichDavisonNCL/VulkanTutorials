/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "Vulkan.h"

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