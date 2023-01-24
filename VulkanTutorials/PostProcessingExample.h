/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class PostProcessingExample : public VulkanTutorialRenderer {
	public:
		PostProcessingExample(Window& window);
		~PostProcessingExample() {}

		void SetupTutorial() override;

		void RenderFrame()		override;	
	protected:
		void	BuildMainPipeline();
		void	BuildProcessPipeline();

		UniqueVulkanMesh gridMesh;
		UniqueVulkanMesh quadMesh;

		UniqueVulkanTexture postTexture;

		UniqueVulkanShader gridShader;
		UniqueVulkanShader invertShader;

		vk::UniqueSampler	sceneSampler;
		vk::UniqueSampler	processSampler;

		VulkanPipeline	gridPipeline;
		VulkanPipeline	invertPipeline;

		vk::UniqueDescriptorSetLayout	processLayout;
		vk::UniqueDescriptorSet			processDescriptor;

		vk::UniqueDescriptorSetLayout	matrixLayout;

		RenderObject gridObject;
	};
}