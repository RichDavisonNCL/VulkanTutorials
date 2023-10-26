/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class OffsetUniformBufferRenderer : public VulkanTutorialRenderer {
	public:
		OffsetUniformBufferRenderer(Window& window);
		~OffsetUniformBufferRenderer();

		void SetupTutorial() override;

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;
		VulkanPipeline		pipeline;

		vk::UniqueDescriptorSet			objectMatrixSet;
		vk::UniqueDescriptorSetLayout	objectMatrixLayout;

		VulkanBuffer	objectMatrixData;
	};
}