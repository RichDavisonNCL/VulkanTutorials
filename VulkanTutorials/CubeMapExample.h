/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class CubeMapExample : public VulkanTutorial
	{
	public:
		CubeMapExample(Window& window);
		~CubeMapExample() {}
	protected:
		void RenderFrame(float dt) override;
		UniqueVulkanMesh		quadMesh;
		UniqueVulkanMesh		sphereMesh;

		UniqueVulkanShader		skyboxShader;
		VulkanPipeline			skyboxPipeline;

		UniqueVulkanShader		objectShader;
		VulkanPipeline			objectPipeline;

		UniqueVulkanTexture		cubeTex;
		VulkanBuffer			camPosUniform; 

		vk::UniqueDescriptorSet	cubemapDescriptor;
		vk::UniqueDescriptorSet	cameraPosDescriptor;
	};
}

