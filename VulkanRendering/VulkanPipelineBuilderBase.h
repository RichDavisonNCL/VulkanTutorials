/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipeline.h"
#include "VulkanShaderModule.h"
#include "VulkanUtils.h"
#include "SmartTypes.h"

namespace NCL::Rendering::Vulkan {
	class VulkanRenderer;
	class VulkanShader;

	struct VulkanVertexSpecification;

	template <class T, class P>
	class PipelineBuilderBase	{
	public:

		T& WithLayout(vk::PipelineLayout pipeLayout) {
			m_layout = pipeLayout;
			m_pipelineCreate.setLayout(pipeLayout);
			return (T&)*this;
		}

		T& WithDescriptorSetLayout(uint32_t setIndex, const vk::UniqueDescriptorSetLayout& m_layout) {
			return WithDescriptorSetLayout(setIndex, *m_layout);
		}

		T& WithDescriptorSetLayout(uint32_t setIndex, vk::DescriptorSetLayout m_layout) {
			assert(setIndex < 32);
			if (setIndex >= m_userLayouts.size()) {
				vk::DescriptorSetLayout nullLayout = Vulkan::GetNullDescriptor(m_sourceDevice);
				while (m_userLayouts.size() <= setIndex) {
					m_userLayouts.push_back(nullLayout);
				}
			}
			m_userLayouts[setIndex] = m_layout;
			return (T&)*this;
		}

		T& WithDescriptorSetLayoutCreationFlags(uint32_t setIndex, vk::DescriptorSetLayoutCreateFlags flags) {
			m_userLayoutCreationFlags[setIndex] = flags;
			return (T&)*this;
		}

		T& WithCreationFlags(vk::PipelineCreateFlagBits flags) {
			m_pipelineCreate.flags |= flags;
			return (T&)*this;
		}

		T& WithCreationFlags(vk::PipelineCreateFlagBits2 flags) {
			m_pipelineCreateBits |= flags;
			return (T&)*this;
		}

		P& GetCreateInfo() {
			return m_pipelineCreate;
		}

		/* SDK 1.4.328 removed ShaderDescriptorSetAndBindingMappingInfoEXT
		T& WithDescriptorSetAndBindingMappingInfo(const vk::ShaderDescriptorSetAndBindingMappingInfoEXT& info) {
			m_heapBindingInfo = info;
			return (T&)*this;
		}
		*/

	protected:
		PipelineBuilderBase(vk::Device device) {
			m_sourceDevice = device;
		}
		~PipelineBuilderBase() = default;

		void FillShaderState(VulkanPipeline& output) {
			for (int i = 0; i < m_usedModules.size(); ++i) {
				vk::PipelineShaderStageCreateInfo stageInfo;

				stageInfo.pName = m_moduleEntryPoints[i].c_str();
				stageInfo.stage = m_usedModules[i]->m_shaderStage;
				stageInfo.module = *m_usedModules[i]->m_shaderModule;

				m_shaderStages.push_back(stageInfo);
			}

			m_pipelineCreate.setStageCount(m_shaderStages.size());
			m_pipelineCreate.setPStages(m_shaderStages.data());

			if (m_externalLayout) {
				m_pipelineCreate.setLayout(m_externalLayout);
			}
			else {
				for (auto& module : m_usedModules) {
					module->CombineLayoutBindings(output.m_allLayoutsBindings);
					module->CombinePushConstantRanges(output.m_pushConstants);
				}
				FinaliseLayout(output);
			}
		}

		void FinaliseLayout(VulkanPipeline& output) {
			/* eDescriptorHeapEXT removed in SDK 1.4.328
			if (m_pipelineCreateBits & vk::PipelineCreateFlagBits2::eDescriptorHeapEXT) {
				return;
			}
			*/

			output.m_allLayouts.resize(output.m_allLayoutsBindings.size());
			for (int i = 0; i < output.m_allLayoutsBindings.size(); ++i) {
				if (i < m_userLayouts.size() && m_userLayouts[i]) {
					output.m_allLayouts[i] = m_userLayouts[i];
				}
				else {
					vk::DescriptorSetLayoutCreateInfo createInfo;
					createInfo.setBindings(output.m_allLayoutsBindings[i]);

					auto userFlags = m_userLayoutCreationFlags.find(i);

					if (userFlags != m_userLayoutCreationFlags.end()) {
						createInfo.flags |= userFlags->second;
					}
					output.m_createdLayouts.push_back(m_sourceDevice.createDescriptorSetLayoutUnique(createInfo));
					output.m_allLayouts[i] = output.m_createdLayouts.back().get();
				}
			}
			vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();
			pipeLayoutCreate.setSetLayouts(output.m_allLayouts);
			pipeLayoutCreate.setPushConstantRanges(output.m_pushConstants);
			output.layout = m_sourceDevice.createPipelineLayoutUnique(pipeLayoutCreate);
			m_pipelineCreate.setLayout(*output.layout);
		}

	protected:
		P m_pipelineCreate;
		vk::PipelineLayout	m_layout;
		vk::Device			m_sourceDevice;

		vk::PipelineCreateFlags2				m_pipelineCreateBits;

		/* SDK 1.4.328 removed ShaderDescriptorSetAndBindingMappingInfoEXT
		vk::ShaderDescriptorSetAndBindingMappingInfoEXT m_heapBindingInfo;
		*/

		vk::PipelineLayout						m_externalLayout;

		std::map<uint32_t, vk::DescriptorSetLayoutCreateFlags> m_userLayoutCreationFlags;

		std::vector< vk::DescriptorSetLayout>	m_userLayouts;

		std::vector<vk::PipelineShaderStageCreateInfo>	m_shaderStages;
		std::vector<const VulkanShaderModule*>			m_usedModules;
		std::vector<UniqueVulkanShaderModule>			m_loadedShaderModules;
		std::vector<std::string>						m_moduleEntryPoints;
	};
}