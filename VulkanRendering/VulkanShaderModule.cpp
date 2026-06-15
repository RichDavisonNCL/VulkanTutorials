/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanShaderModule.h"
#include "Assets.h"
#include "VulkanUtils.h"

extern "C" {
#include "Spirv-reflect/Spirv_reflect.h"
}

using std::ifstream;

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanShaderModule::VulkanShaderModule(const std::string& filename, vk::ShaderStageFlagBits stage, vk::Device device)	{
	char* data;
	size_t dataSize = 0;
	Assets::ReadBinaryFile(Assets::SHADERDIR + "VK/" + filename, &data, dataSize);

	if (dataSize > 0) {
		m_shaderModule = device.createShaderModuleUnique(
			{
				.flags		= {},
				.codeSize	= dataSize,
				.pCode		= (uint32_t*)data
			}
		);
		AddReflectionData(dataSize, data, stage);
		BuildLayouts(device);

		Vulkan::SetDebugName(device, *m_shaderModule, filename);
	}
	else {
		std::cout << __FUNCTION__ << " Problem loading shader file " << filename << "!\n";
	}
	m_shaderStage = stage;
}

void VulkanShaderModule::CombineLayoutBindings(std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& inoutBindings) const {
	const int numSets = std::max(inoutBindings.size(), m_allLayoutsBindings.size());
	inoutBindings.resize(numSets);

	for (int i = 0; i < m_allLayoutsBindings.size(); ++i) {
		std::vector<vk::DescriptorSetLayoutBinding>& outSet		= inoutBindings[i];
		const std::vector<vk::DescriptorSetLayoutBinding>& baseSet	= m_allLayoutsBindings[i];

		const int numBindings = std::max(outSet.size(), baseSet.size());
		outSet.resize(numBindings);

		for (int j = 0; j < baseSet.size(); ++j) {
			if (baseSet[j].stageFlags != vk::ShaderStageFlags()) {
				//Check that something hasn't gone wrong with the binding combo!
				if (baseSet[j].descriptorType != outSet[j].descriptorType) {

				}
				if (baseSet[j].descriptorCount != outSet[j].descriptorCount) {

				}
				outSet[j].binding			= j;
				outSet[j].descriptorCount	= baseSet[j].descriptorCount;
				outSet[j].descriptorType	= baseSet[j].descriptorType;			
				
				outSet[j].stageFlags |= m_shaderStage; //Combine sets across shader stages
			}
		}
	}
}

void VulkanShaderModule::CombinePushConstantRanges(std::vector< vk::PushConstantRange>& inoutRanges) const {
	for (int i = 0; i < m_pushConstants.size(); ++i) {
		bool found = false;
		for (int j = 0; j < inoutRanges.size(); ++j) {
			if (m_pushConstants[i].offset == inoutRanges[j].offset &&
				m_pushConstants[i].size == inoutRanges[j].size) {
				inoutRanges[j].stageFlags |= m_shaderStage;
				found = true;
				break;
			}
		}
		if (!found) {
			inoutRanges.push_back(m_pushConstants[i]);
		}
	}
}

void VulkanShaderModule::AddReflectionData(uint32_t dataSize, const void* data, vk::ShaderStageFlags stage) {
	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(dataSize, data, &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	uint32_t descriptorCount = 0;
	result = spvReflectEnumerateDescriptorSets(&module, &descriptorCount, NULL);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectDescriptorSet*> descriptorSetLayouts(descriptorCount);
	result = spvReflectEnumerateDescriptorSets(&module, &descriptorCount, descriptorSetLayouts.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	for (auto& set : descriptorSetLayouts) {
		if (set->set >= m_allLayoutsBindings.size()) {
			m_allLayoutsBindings.resize(set->set + 1);
		}
		std::vector<vk::DescriptorSetLayoutBinding>& setLayout = m_allLayoutsBindings[set->set];
		setLayout.resize(set->binding_count);
		for (int i = 0; i < set->binding_count; ++i) {
			SpvReflectDescriptorBinding* binding = set->bindings[i];

			uint32_t index = binding->binding;
			uint32_t count = binding->count;

			if (index >= setLayout.size()) {
				setLayout.resize(index + 1);
			}

			vk::ShaderStageFlags stages;

			if (setLayout[index].stageFlags != vk::ShaderStageFlags()) {
				//Check that something hasn't gone wrong with the binding combo!
				if (setLayout[index].descriptorType != (vk::DescriptorType)binding->descriptor_type) {

				}
				if (setLayout[index].descriptorCount != binding->count) {

				}
			}
			setLayout[index].binding = index;
			setLayout[index].descriptorCount = binding->count;
			setLayout[index].descriptorType = (vk::DescriptorType)binding->descriptor_type;

			setLayout[index].stageFlags |= stage; //Combine sets across shader stages
		}
	}

	uint32_t pushConstantCount = 0;
	result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, NULL);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectBlockVariable*> pushConstantLayouts(pushConstantCount);
	result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstantLayouts.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	for (auto& constant : pushConstantLayouts) {
		for (int i = 0; i < constant->member_count; ++i) {
			auto& member = constant->members[i];
			//Check to see if this one was loaded 
			bool found = false;
			for (int i = 0; i < m_pushConstants.size(); ++i) {
				if (m_pushConstants[i].offset == member.offset &&
					m_pushConstants[i].size == member.size) {
					m_pushConstants[i].stageFlags |= stage;
					found = true;
					break;
				}
			}
			if (!found) {
				vk::PushConstantRange range;
				range.offset = constant->offset;
				range.size = constant->size;
				range.stageFlags = stage;
				m_pushConstants.push_back(range);
			}
		}
	}
	spvReflectDestroyShaderModule(&module);
}

void VulkanShaderModule::BuildLayouts(vk::Device device) {
	for (const auto& i : m_allLayoutsBindings) {
		vk::DescriptorSetLayoutCreateInfo createInfo;
		createInfo.setBindings(i);
		m_reflectionLayouts.push_back(device.createDescriptorSetLayoutUnique(createInfo));
	}
}