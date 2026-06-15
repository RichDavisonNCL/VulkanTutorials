/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanRayTracingPipelineBuilder.h"

#include "VulkanMesh.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanRayTracingPipelineBuilder::VulkanRayTracingPipelineBuilder(vk::Device device) : PipelineBuilderBase(device){
}

VulkanRayTracingPipelineBuilder::~VulkanRayTracingPipelineBuilder() {
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithRecursionDepth(uint32_t count) {
	m_pipelineCreate.maxPipelineRayRecursionDepth = count;
	return *this;
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithRayGenGroup(uint32_t shaderIndex) {
	vk::RayTracingShaderGroupCreateInfoKHR groupCreateInfo;
	groupCreateInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	groupCreateInfo.generalShader = shaderIndex;
	m_genGroups.push_back(groupCreateInfo);
	return *this;
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithMissGroup(uint32_t shaderIndex) {
	vk::RayTracingShaderGroupCreateInfoKHR groupCreateInfo;
	groupCreateInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	groupCreateInfo.generalShader = shaderIndex;
	m_missGroups.push_back(groupCreateInfo);
	return *this;
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithTriangleHitGroup(uint32_t closestHit, uint32_t anyHit) {
	vk::RayTracingShaderGroupCreateInfoKHR groupCreateInfo;

	groupCreateInfo.type				= vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
	groupCreateInfo.generalShader		= VK_SHADER_UNUSED_KHR;
	groupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_KHR;
	groupCreateInfo.closestHitShader	= closestHit;
	groupCreateInfo.anyHitShader		= anyHit;

	m_hitGroups.push_back(groupCreateInfo);

	return *this;
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithProceduralHitGroup(uint32_t intersection, uint32_t closestHit, uint32_t anyHit){
	vk::RayTracingShaderGroupCreateInfoKHR groupCreateInfo;

	groupCreateInfo.type				= vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
	groupCreateInfo.generalShader		= VK_SHADER_UNUSED_KHR;
	groupCreateInfo.intersectionShader	= intersection;
	groupCreateInfo.closestHitShader	= closestHit;
	groupCreateInfo.anyHitShader		= anyHit;

	m_hitGroups.push_back(groupCreateInfo);

	return *this;
}

VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithShaderBinary(const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entrypoint) {
	m_loadedShaderModules.push_back(std::make_unique<VulkanShaderModule>(filename, stage, m_sourceDevice));
	m_usedModules.push_back(m_loadedShaderModules.back().get());
	m_moduleEntryPoints.push_back(entrypoint);

	return *this;
}


VulkanRayTracingPipelineBuilder& VulkanRayTracingPipelineBuilder::WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint) {
	m_usedModules.push_back(&module);
	m_moduleEntryPoints.push_back(entrypoint);

	return *this;
}

VulkanPipeline VulkanRayTracingPipelineBuilder::Build(const std::string& debugName, vk::PipelineCache cache) {
	VulkanPipeline output;	
	
	FillShaderState(output);
	
	m_allGroups.clear();
	m_allGroups.insert(m_allGroups.end(), m_genGroups.begin() , m_genGroups.end());
	m_allGroups.insert(m_allGroups.end(), m_missGroups.begin(), m_missGroups.end());
	m_allGroups.insert(m_allGroups.end(), m_hitGroups.begin() , m_hitGroups.end());

	m_pipelineCreate.groupCount	= m_allGroups.size();
	m_pipelineCreate.pGroups	= m_allGroups.data();

	m_pipelineCreate.setPDynamicState(&m_dynamicCreate);

	output.pipeline = m_sourceDevice.createRayTracingPipelineKHRUnique({}, cache, m_pipelineCreate).value;

	if (!debugName.empty()) {
		SetDebugName(m_sourceDevice, *output.pipeline	, debugName);
		SetDebugName(m_sourceDevice, *output.layout		, debugName);
	}

	return output;
}