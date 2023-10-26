/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class PrerecordedCmdListRenderer : public VulkanTutorialRenderer	{
	public:
		PrerecordedCmdListRenderer(Window& window);
		~PrerecordedCmdListRenderer();

		void SetupTutorial() override;

		void RenderFrame() override;

	protected:
		void BuildPipeline();
		VulkanPipeline		pipeline;

		UniqueVulkanMesh		triMesh;
		UniqueVulkanShader		shader;

		vk::CommandBuffer	recordedBuffer;
	};
}