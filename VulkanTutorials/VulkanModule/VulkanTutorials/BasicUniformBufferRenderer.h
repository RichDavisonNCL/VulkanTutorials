/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Plugins/VulkanRendering/VulkanRenderer.h"
#include "../../Plugins/VulkanRendering/VulkanPipeline.h"
#include "../../Common/Camera.h"

namespace NCL::Rendering {
	class BasicUniformBufferRenderer : public VulkanRenderer	{
	public:
		BasicUniformBufferRenderer(Window& window);
		~BasicUniformBufferRenderer();

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		void	BuildPipeline();
		void	UpdateCameraUniform();

		void	WriteDescriptorSet(vk::DescriptorSet s);

		std::shared_ptr<VulkanMesh>		triMesh;
		std::shared_ptr<VulkanShader>	defaultShader;
		VulkanPipeline	basicPipeline;

		vk::UniqueDescriptorSet	descriptorSet;

		VulkanBuffer	cameraData;
		Camera			camera;

		Matrix4* cameraMemory;
	};
}