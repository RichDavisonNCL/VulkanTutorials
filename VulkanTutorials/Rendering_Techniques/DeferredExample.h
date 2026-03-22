/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class DeferredExample : public VulkanTutorial {

	public:
		DeferredExample(Window& window, VulkanInitialisation& vkInit);
		~DeferredExample() {}

		void OnWindowResize(uint32_t width, uint32_t height) override;

	protected:
		void	RenderFrame(float dt) override;
		void	CreateFrameBuffers(uint32_t width, uint32_t height);
		void	BuildPipelines();


		void	FillGBuffer();
		void	RenderLights();
		void	CombineBuffers();
		void	UpdateDescriptors();

		VulkanBuffer lightUniform;
		VulkanBuffer lightStageUniform;

		RenderObject boxObject;
		RenderObject floorObject;

		enum ScreenTextures {
			Albedo,
			Normals,
			Depth,
			Diffuse,
			Specular,
			MAX_TEXTURES
		};

		enum Descriptors {
			Lighting,
			Combine,
			LightState,
			LightTexture,
			MAX_DESCRIPTORS
		};

		UniqueVulkanTexture bufferTextures[ScreenTextures::MAX_TEXTURES];
		UniqueVulkanTexture objectTextures[4];

		vk::UniqueDescriptorSet	descriptorSets[Descriptors::MAX_DESCRIPTORS];

		VulkanPipeline	gBufferPipeline;
		VulkanPipeline	lightPipeline;
		VulkanPipeline	combinePipeline;
	};
}