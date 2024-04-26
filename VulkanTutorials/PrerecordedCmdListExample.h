/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
	class PrerecordedCmdListExample : public VulkanTutorial	{
	public:
		PrerecordedCmdListExample(Window& window);
		~PrerecordedCmdListExample();

//		void SetupTutorial() override;

		//void RenderFrame() override;

	protected:
		void BuildPipeline();
		VulkanPipeline		pipeline;

		UniqueVulkanMesh		triMesh;
		UniqueVulkanShader		shader;

		vk::CommandBuffer	recordedBuffer;
	};
}