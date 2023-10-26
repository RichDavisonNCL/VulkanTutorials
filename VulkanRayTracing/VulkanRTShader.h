/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering {
	class VulkanRTShader
	{
	public:
		VulkanRTShader(const std::string& filename, vk::Device device);
	
		const vk::UniqueShaderModule& GetModule() const {
			return shaderModule;
		}

	protected:
		vk::UniqueShaderModule shaderModule;
		std::string entryPoint;

	
	};
}