/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/ShaderBase.h"
namespace NCL::Rendering {
	class VulkanShader : public ShaderBase {
	public:
		friend class VulkanRenderer;
		friend class VulkanShaderBuilder;

		void ReloadShader() override;

		void	FillShaderStageCreateInfo(vk::GraphicsPipelineCreateInfo& info) const;
		~VulkanShader();

	protected:
		void AddBinaryShaderModule(const string& fromFile, ShaderStages stage, vk::Device device, const string& entryPoint = "main");

		void Init();

	protected:			
		VulkanShader();

		vk::UniqueShaderModule shaderModules[(int)ShaderStages::MAXSIZE];
		string entryPoints[(int)ShaderStages::MAXSIZE];

		uint32_t stageCount;
		vk::PipelineShaderStageCreateInfo* infos;
	};
}