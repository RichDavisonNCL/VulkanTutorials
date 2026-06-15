/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipeline.h"
#include "VulkanShaderModule.h"
#include "VulkanPipelineBuilderBase.h"
#include "SmartTypes.h"

namespace NCL::Rendering::Vulkan {
	class VulkanCompute;
	/*
	ComputePipelineBuilder: A Builder class to automate the creation of 
	compute pipelines, including the correct push constants and descriptor
	set layouts, obtained from the shader module via reflection. 
	*/
	class ComputePipelineBuilder : public PipelineBuilderBase<ComputePipelineBuilder, vk::ComputePipelineCreateInfo>	{
	public:
		ComputePipelineBuilder(vk::Device m_device);
		~ComputePipelineBuilder() {}

		ComputePipelineBuilder& WithShaderBinary(const std::string& filename, const std::string& entrypoint = "main");
		ComputePipelineBuilder& WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint = "main");

		VulkanPipeline	Build(const std::string& debugName = "", vk::PipelineCache cache = {});

	protected:
		const VulkanShaderModule*	m_module;
		std::string			m_entryPoint;
	};
};