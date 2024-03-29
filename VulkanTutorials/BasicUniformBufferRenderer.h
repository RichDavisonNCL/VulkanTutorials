/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class BasicUniformBufferRenderer : public VulkanTutorialRenderer {
	public:
		BasicUniformBufferRenderer(Window& window);
		~BasicUniformBufferRenderer();

		void SetupTutorial() override;

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		void	UpdateCameraUniform();

		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;
		VulkanPipeline		pipeline;

		vk::UniqueDescriptorSet			descriptorSet;
		vk::UniqueDescriptorSetLayout	matrixLayout;

		VulkanBuffer		cameraData;
		PerspectiveCamera	camera;
		Matrix4*			cameraMemory;
	};
}