/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class UniformBufferExample : public VulkanTutorial {
	public:
		UniformBufferExample(Window& window);
		~UniformBufferExample();

	protected:
		void RenderFrame(float dt) override;
		void	UpdateCameraUniform();

		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;
		VulkanPipeline		pipeline;

		vk::UniqueDescriptorSet			descriptorSet;

		VulkanBuffer		cameraData;
		PerspectiveCamera	camera;
		Matrix4*			cameraMemory;
	};
}