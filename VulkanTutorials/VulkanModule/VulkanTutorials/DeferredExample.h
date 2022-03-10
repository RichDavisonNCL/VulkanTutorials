/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class DeferredExample : public VulkanTutorialRenderer {

	public:
		DeferredExample(Window& window);
		~DeferredExample();

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		void	LoadShaders();
		void	CreateFrameBuffers();
		void	BuildGBufferPipeline();
		void	BuildLightPassPipeline();
		void	BuildCombinePipeline();

		void	FillGBuffer();
		void	RenderLights();
		void	CombineBuffers();

		std::unique_ptr<VulkanShader> gBufferShader;
		std::unique_ptr<VulkanShader> lightingShader;
		std::unique_ptr<VulkanShader> combineShader;

		VulkanBuffer lightUniform;
		VulkanBuffer lightStageUniform;

		RenderObject boxObject;
		RenderObject floorObject;

		std::shared_ptr<VulkanMesh> sphereMesh;
		std::shared_ptr<VulkanMesh> cubeMesh;
		std::shared_ptr<VulkanMesh> quadMesh;

		std::shared_ptr<VulkanTexture> bufferTextures[5];
		std::shared_ptr<VulkanTexture> objectTextures[4];

		vk::UniqueDescriptorSet			lightDescriptor;
		vk::UniqueDescriptorSetLayout	lightLayout;

		vk::UniqueDescriptorSet			lightStageCamDescriptor;
		vk::UniqueDescriptorSetLayout	lightStageCamLayout;

		vk::UniqueDescriptorSetLayout	lightCameraLayout;

		vk::UniqueDescriptorSetLayout	combineTextureLayout;
		vk::UniqueDescriptorSet			combineTextureDescriptor;

		vk::UniqueDescriptorSet			lightStageDescriptor;
		vk::UniqueDescriptorSetLayout	lightStageLayout;
		vk::UniqueDescriptorSetLayout	lightTextureLayout;

		vk::UniqueDescriptorSet			lightTextureDescriptor;

		vk::UniqueDescriptorSetLayout	objectTextureLayout;

		VulkanPipeline	gBufferPipeline;
		VulkanPipeline	lightPipeline;
		VulkanPipeline	combinePipeline;
	};
}