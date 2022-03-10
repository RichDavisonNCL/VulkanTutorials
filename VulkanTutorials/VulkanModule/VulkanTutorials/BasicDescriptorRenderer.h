/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"
#include "../../Common/Vector3.h"
#include "../../Common/Vector4.h"
#include "../../Plugins/VulkanRendering/VulkanPipeline.h"
namespace NCL::Rendering {
	class BasicDescriptorRenderer : public VulkanTutorialRenderer	{
	public:
		BasicDescriptorRenderer(Window& window);
		~BasicDescriptorRenderer();

		void RenderFrame() override;

	protected:
		void	BuildPipeline();

		VulkanPipeline	pipeline;

		std::shared_ptr<VulkanMesh>		triMesh;
		std::shared_ptr<VulkanShader>	shader;

		VulkanBuffer	uniformData[2];

		vk::UniqueDescriptorSet			descriptorSet;
		vk::UniqueDescriptorSetLayout	descriptorLayout;

		Vector4	colourUniform;
		Vector3 positionUniform;
	};
}