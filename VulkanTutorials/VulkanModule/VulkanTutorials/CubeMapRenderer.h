/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class CubeMapRenderer : public VulkanTutorialRenderer
	{
	public:
		CubeMapRenderer(Window& window);
		~CubeMapRenderer();

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		VulkanMesh*		quadMesh;
		VulkanMesh*		sphereMesh;

		std::unique_ptr<VulkanShader>	skyboxShader;
		VulkanPipeline	skyboxPipeline;

		std::unique_ptr<VulkanShader>	objectShader;
		VulkanPipeline	objectPipeline;

		std::shared_ptr<VulkanTexture>	cubeTex;

		vk::UniqueDescriptorSetLayout cubemapLayout;

		vk::UniqueDescriptorSet	cubemapDescriptor;

		vk::UniqueDescriptorSet			cameraPosDescriptor;
		vk::UniqueDescriptorSetLayout	cameraPosLayout;
		VulkanBuffer camPosUniform; 
	};
}

