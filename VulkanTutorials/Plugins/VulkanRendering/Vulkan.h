/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
namespace NCL::Rendering::Vulkan {
		void SetDebugName(vk::Device d,	vk::ObjectType t, uint64_t handle, const std::string& debugName);

		extern vk::DispatchLoaderDynamic* dispatcher;
}