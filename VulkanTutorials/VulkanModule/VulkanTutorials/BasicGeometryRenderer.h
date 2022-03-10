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
	class BasicGeometryRenderer : public VulkanRenderer	{
	public:
		BasicGeometryRenderer(Window& window);
		~BasicGeometryRenderer();

	protected:
		virtual void RenderFrame();

		void	BuildPipeline();

		std::shared_ptr<VulkanMesh>		triMesh;
		std::shared_ptr<VulkanShader>	shader;
		VulkanPipeline	basicPipeline;
	};
}