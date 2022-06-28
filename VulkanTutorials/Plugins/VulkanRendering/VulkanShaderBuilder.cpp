/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanUtils.h"
#include "VulkanShader.h"
#include "VulkanShaderBuilder.h"

using std::string;
using namespace NCL;
using namespace Rendering;

VulkanShaderBuilder& VulkanShaderBuilder::WithMeshBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Mesh] = name;
	entryPoints[(int)ShaderStages::Mesh] = entry;
	return *this;
}

VulkanShaderBuilder& VulkanShaderBuilder::WithVertexBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Vertex] = name;
	entryPoints[(int)ShaderStages::Vertex] = entry;
	return *this;
}

VulkanShaderBuilder& VulkanShaderBuilder::WithFragmentBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Fragment] = name;
	entryPoints[(int)ShaderStages::Fragment] = entry;
	return *this;
}

VulkanShaderBuilder& VulkanShaderBuilder::WithGeometryBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Geometry] = name;
	entryPoints[(int)ShaderStages::Geometry] = entry;
	return *this;
}

VulkanShaderBuilder& VulkanShaderBuilder::WithTessControlBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Domain] = name;
	entryPoints[(int)ShaderStages::Domain] = entry;
	return *this;
}

VulkanShaderBuilder& VulkanShaderBuilder::WithTessEvalBinary(const string& name, const std::string& entry) {
	shaderFiles[(int)ShaderStages::Hull] = name;
	entryPoints[(int)ShaderStages::Hull] = entry;
	return *this;
}

UniqueVulkanShader VulkanShaderBuilder::Build(vk::Device device) {
	VulkanShader* newShader = new VulkanShader();

	//mesh and 'traditional' pipeline are mutually exclusive
	if (!shaderFiles[(int)ShaderStages::Mesh].empty() &&
		!shaderFiles[(int)ShaderStages::Vertex].empty()
	) {
		//TODO formalize error message?
		return nullptr;
	}

	for (int i = 0; i < (int)ShaderStages::MAXSIZE; ++i) {
		if (!shaderFiles[i].empty()) {
			newShader->AddBinaryShaderModule(shaderFiles[i],(ShaderStages)i, device, entryPoints[i]);

			if (!debugName.empty()) {
				Vulkan::SetDebugName(device, vk::ObjectType::eShaderModule, Vulkan::GetVulkanHandle(*newShader->shaderModules[i]), debugName);
			}
		}
	};
	newShader->Init();
	return UniqueVulkanShader(newShader);
}