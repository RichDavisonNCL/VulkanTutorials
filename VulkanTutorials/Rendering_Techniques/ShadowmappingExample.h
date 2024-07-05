/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class ShadowMappingExample : public VulkanTutorial	{
	public:
		ShadowMappingExample(Window& window, VulkanInitialisation& vkInit);
		~ShadowMappingExample() {}
	protected:
		void RenderFrame(float dt) override;

		void	RenderShadowMap();
		void	RenderScene();

		void DrawObjects(vk::CommandBuffer buffer, VulkanPipeline& pipeline);

		UniqueVulkanMesh	cubeMesh;

		UniqueVulkanShader	shadowFillShader;
		UniqueVulkanShader	shadowUseShader;

		VulkanPipeline	shadowPipeline;
		VulkanPipeline	scenePipeline;

		UniqueVulkanTexture shadowMap;

		RenderObject	sceneObjects[2];

		vk::UniqueDescriptorSet	sceneShadowTexDescriptor;
		vk::UniqueDescriptorSet shadowMatrixDescriptor;

		VulkanBuffer shadowMatBuffer;
		Matrix4*	 shadowMatrix;
	};
}