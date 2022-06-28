/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/ShaderBase.h"
#include "SmartTypes.h"

namespace NCL::Rendering {
	class VulkanShader;
	class VulkanShaderBuilder {
	public:
		VulkanShaderBuilder(const std::string& name = "") { debugName = name; };
		~VulkanShaderBuilder()	{};

		VulkanShaderBuilder& WithMeshBinary(const std::string& name, const std::string& entry = "main");

		VulkanShaderBuilder& WithVertexBinary(const std::string& name, const std::string& entry = "main");
		VulkanShaderBuilder& WithFragmentBinary(const std::string& name, const std::string& entry = "main");
		VulkanShaderBuilder& WithGeometryBinary(const std::string& name, const std::string& entry = "main");
		VulkanShaderBuilder& WithTessControlBinary(const std::string& name, const std::string& entry = "main");
		VulkanShaderBuilder& WithTessEvalBinary(const std::string& name, const std::string& entry = "main");

		UniqueVulkanShader Build(vk::Device device);

	protected:
		std::string shaderFiles[(int)ShaderStages::MAXSIZE];
		std::string entryPoints[(int)ShaderStages::MAXSIZE];
		std::string debugName;
	};
}