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
		~DeferredExample() {}

		void SetupTutorial() override;

		void RenderFrame()		override;
		void Update(float dt)	override;
		void OnWindowResize(int w, int h)	override;

	protected:
		void	LoadShaders();
		void	CreateFrameBuffers(uint32_t width, uint32_t height);
		void	BuildGBufferPipeline();
		void	BuildLightPassPipeline();
		void	BuildCombinePipeline();

		void	FillGBuffer();
		void	RenderLights();
		void	CombineBuffers();
		void	UpdateDescriptors();

		UniqueVulkanShader gBufferShader;
		UniqueVulkanShader lightingShader;
		UniqueVulkanShader combineShader;

		VulkanBuffer lightUniform;
		VulkanBuffer lightStageUniform;

		RenderObject boxObject;
		RenderObject floorObject;

		UniqueVulkanMesh sphereMesh;
		UniqueVulkanMesh cubeMesh;
		UniqueVulkanMesh quadMesh;

		UniqueVulkanTexture bufferTextures[5];
		UniqueVulkanTexture objectTextures[4];

		vk::UniqueDescriptorSet			lightDescriptor;
		vk::UniqueDescriptorSetLayout	lightLayout;

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