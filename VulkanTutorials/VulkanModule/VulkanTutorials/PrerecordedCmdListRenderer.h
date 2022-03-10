/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Plugins/VulkanRendering/VulkanRenderer.h"
#include "../../Plugins/VulkanRendering/VulkanPipeline.h"
namespace NCL::Rendering {
	class PrerecordedCmdListRenderer : public VulkanRenderer	{
	public:
		PrerecordedCmdListRenderer(Window& window);
		~PrerecordedCmdListRenderer();

		void RenderFrame() override;

	protected:
		void BuildPipeline();
		VulkanPipeline		pipeline;

		VulkanMesh*			triMesh;
		VulkanShader*		shader;

		vk::CommandBuffer	recordedBuffer;
	};
}