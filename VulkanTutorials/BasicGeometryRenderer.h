/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering::Vulkan {
	class BasicGeometryRenderer : public VulkanTutorialRenderer	{
	public:
		BasicGeometryRenderer(Window& window);
		~BasicGeometryRenderer() {} //Nothing to delete in this one!

		void SetupTutorial() override;

	protected:
		virtual void RenderFrame();

		UniqueVulkanMesh 	triMesh;
		UniqueVulkanShader	shader;
		VulkanPipeline		basicPipeline;
	};
}