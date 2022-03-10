/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Plugins/VulkanRendering/VulkanRenderer.h"
#include "../../Plugins/VulkanRendering/VulkanBVHBuilder.h"

namespace NCL::Rendering {
	class TestRayTrace : public VulkanRenderer
	{
	public:
		TestRayTrace(Window& window);
		~TestRayTrace();

		void RenderFrame() override;
		void Update(float dt) override;

	protected:
		VulkanPipeline	pipeline;
		VulkanMesh*		triMesh;
		VulkanBVH		sceneBVH;
	};
}

