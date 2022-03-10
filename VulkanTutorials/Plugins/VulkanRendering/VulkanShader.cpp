/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanShader.h"
#include "../../Common/Assets.h"

#include <fstream>
#include <iostream>
#include <string>
#include <iosfwd>
#include <set>

using std::ifstream;

using namespace NCL;
using namespace Rendering;

//These have both been ordered to match the ShaderStages enum for easy lookup!
vk::ShaderStageFlagBits stageTypes[] = {
	vk::ShaderStageFlagBits::eVertex,
	vk::ShaderStageFlagBits::eFragment, 
	vk::ShaderStageFlagBits::eGeometry,
	vk::ShaderStageFlagBits::eTessellationControl,
	vk::ShaderStageFlagBits::eTessellationEvaluation,
	vk::ShaderStageFlagBits::eMeshNV
};

VulkanShader::VulkanShader()	{
	stageCount	= 0;
	infos		= nullptr;
}

VulkanShader::~VulkanShader()	{
	std::set< vk::ShaderModule> uniqueModules;

	for (int i = 0; i < stageCount; ++i) {
		uniqueModules.insert(shaderModules[i]);	
	}

	for (vk::ShaderModule i : uniqueModules) {
		sourceDevice.destroyShaderModule(i);
	}
	delete[] infos;
}

void VulkanShader::ReloadShader() {

}

void VulkanShader::SetSourceDevice(vk::Device d) {
	sourceDevice = d;
}

void VulkanShader::AddBinaryShaderModule(const string& fromFile, ShaderStages stage, const string& entryPoint) {
	char* data;
	size_t dataSize = 0;
	Assets::ReadBinaryFile(Assets::SHADERDIR + "VK/" + fromFile, &data, dataSize);
	bool found = false;
	for (int i = 0; i < (int)ShaderStages::MAXSIZE; ++i) {
		if (shaderFiles[i] == fromFile) {	//already loaded this binary!
			shaderModules[(int)stage] = shaderModules[i];
			found = true;
			break;
		}
	}
	if (!found) {
		if (data) {
			CreateShaderModule(data, dataSize, shaderModules[(int)stage], sourceDevice);
		}
		else {
			std::cout << __FUNCTION__ << " Problem loading shader file " << fromFile << "!\n";
		}
	}
	shaderFiles[(int)stage] = fromFile;
	entryPoints[(int)stage] = entryPoint;
}

void VulkanShader::Init() {
	stageCount = 0;
	for (int i = 0; i < (int)ShaderStages::MAXSIZE; ++i) {
		if (shaderModules[i]) {
			stageCount++;
		}
	}
	infos = new vk::PipelineShaderStageCreateInfo[stageCount];

	int doneCount = 0;
	for (int i = 0; i < (int)ShaderStages::MAXSIZE; ++i) {
		if (shaderModules[i]) {
			infos[doneCount].stage	= stageTypes[i];
			infos[doneCount].module = shaderModules[i];
			infos[doneCount].pName	= entryPoints[i].c_str();

			doneCount++;
			if (doneCount >= stageCount) {
				break;
			}
		}
	}
}

vk::ShaderModule VulkanShader::GetShaderModule(ShaderStages stage) const {
	return shaderModules[(int)stage];
}

bool		VulkanShader::CreateShaderModule(char*data, size_t size, vk::ShaderModule& into, vk::Device& device) {
	into = device.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), size, (uint32_t*)data));
	return true;
}

void	VulkanShader::FillShaderStageCreateInfo(vk::GraphicsPipelineCreateInfo &info) const {
	info.setStageCount(stageCount);
	info.setPStages(infos);
}