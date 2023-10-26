/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanRTShader.h"
#include "Assets.h"

using namespace NCL;
using namespace Rendering;

VulkanRTShader::VulkanRTShader(const std::string& filename, vk::Device device) {
	char* data;
	size_t dataSize = 0;
	Assets::ReadBinaryFile(Assets::SHADERDIR + "VK/" + filename, &data, dataSize);

	if (dataSize > 0) {
		shaderModule = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), dataSize, (uint32_t*)data));
	}
	else {
		std::cout << __FUNCTION__ << " Problem loading shader file " << filename << "!\n";
	}
}