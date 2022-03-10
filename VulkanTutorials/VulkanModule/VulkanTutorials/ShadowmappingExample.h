/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
	class ShadowMappingExample : public VulkanTutorialRenderer	{
	public:
		ShadowMappingExample(Window& window);
		~ShadowMappingExample();

		void RenderFrame()		override;
		void Update(float dt)	override;

	protected:
		void	BuildMainPipeline();
		void	BuildShadowPipeline();

		void	RenderShadowMap();
		void	RenderScene();

		void DrawObjects(VulkanPipeline& toPipeline);

		VulkanMesh*		cubeMesh;

		std::unique_ptr<VulkanShader>	shadowFillShader;
		std::unique_ptr<VulkanShader>	shadowUseShader;

		VulkanPipeline	shadowPipeline;
		VulkanPipeline	scenePipeline;

		VulkanFrameBuffer shadowBuffer;

		RenderObject	boxObject;
		RenderObject	floorObject;

		vk::RenderPass		shadowPass;

		vk::UniqueDescriptorSetLayout shadowTexLayout;
		vk::UniqueDescriptorSetLayout shadowMatrixLayout;
		vk::UniqueDescriptorSetLayout diffuseLayout;

		vk::UniqueDescriptorSet	sceneShadowTexDescriptor;
		vk::UniqueDescriptorSet shadowMatrixDescriptor;

		VulkanBuffer shadowUniform;
		Matrix4*	shadowMatrix;

		vk::Viewport	shadowViewport;
		vk::Rect2D		shadowScissor;
	};
}