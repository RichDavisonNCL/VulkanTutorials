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
		void AddBinaryShaderModule(const string& fromFile, ShaderStages stage, const string& entryPoint = "main");
		vk::ShaderModule GetShaderModule(ShaderStages stage) const;

		void SetSourceDevice(vk::Device d);

		void Init();

	protected:			
		VulkanShader();

		static bool		CreateShaderModule(char* data, size_t size, vk::ShaderModule& into, vk::Device& device);

		vk::ShaderModule shaderModules[(int)ShaderStages::MAXSIZE];
		string entryPoints[(int)ShaderStages::MAXSIZE];
		int stageCount;
		vk::PipelineShaderStageCreateInfo* infos;
		vk::Device sourceDevice;
	};
}