/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
    class MultiViewportExample : public VulkanTutorial
    {
    public:
        MultiViewportExample(Window& window);
        ~MultiViewportExample() {}

    protected:
        void RenderFrame(float dt) override;

        UniqueVulkanMesh    triMesh;
        UniqueVulkanShader  shader;
        VulkanPipeline	    pipeline;
    };
}

