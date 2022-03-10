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
		~PostProcessingExample();

		void RenderFrame()		override;	
	protected:
		void	BuildMainPipeline();
		void	BuildProcessPipeline();

		void	LoadShaders();

		std::shared_ptr<VulkanMesh> gridMesh;
		std::shared_ptr<VulkanMesh> quadMesh;

		VulkanFrameBuffer sceneBuffer;

		std::shared_ptr<VulkanShader> gridShader;
		std::shared_ptr<VulkanShader> invertShader;

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