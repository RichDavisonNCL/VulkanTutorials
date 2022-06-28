/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
namespace NCL::Rendering::Vulkan {

		template <typename T>
		uint64_t GetVulkanHandle(T const& cppHandle) {
			return uint64_t(static_cast<T::CType>(cppHandle));
		}

		void SetDebugName(vk::Device d,	vk::ObjectType t, uint64_t handle, const std::string& debugName);

		void BeginDebugArea(vk::CommandBuffer b, const std::string& name);
		void EndDebugArea(vk::CommandBuffer b);

		extern vk::DispatchLoaderDynamic* dispatcher;
}