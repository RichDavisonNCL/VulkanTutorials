/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanCompute.h"
#include "../../Common/Assets.h"

using namespace NCL::Rendering;

VulkanCompute::VulkanCompute(vk::Device device, const std::string& filename) : localThreadSize{ 0,0,0 } {
	char* data;
	size_t dataSize = 0;
	Assets::ReadBinaryFile(Assets::SHADERDIR + "VK/" + filename, &data, dataSize);

	if (dataSize > 0) {
		computeModule = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), dataSize, (uint32_t*)data));
	}
	else {
		std::cout << __FUNCTION__ << " Problem loading shader file " << filename << "!\n";
		return;
	}
	info.stage	= vk::ShaderStageFlagBits::eCompute;
	info.module = *computeModule;
	info.pName	= "main";
}

int VulkanCompute::GetThreadXCount() const {
	return localThreadSize[0];
}

int VulkanCompute::GetThreadYCount() const {
	return localThreadSize[1];
}

int VulkanCompute::GetThreadZCount() const {
	return localThreadSize[2];
}

void	VulkanCompute::FillShaderStageCreateInfo(vk::ComputePipelineCreateInfo& pipeInfo) const {
	pipeInfo.setStage(info);
}