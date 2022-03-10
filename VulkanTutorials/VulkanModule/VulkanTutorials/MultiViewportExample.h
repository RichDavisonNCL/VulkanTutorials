/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
    class MultiViewportExample : public VulkanTutorialRenderer
    {
    public:
        MultiViewportExample(Window& window);
        ~MultiViewportExample();

    protected:
        virtual void RenderFrame();

        std::shared_ptr<VulkanMesh>     triMesh;
        std::shared_ptr<VulkanShader>   shader;
        VulkanPipeline	basicPipeline;
    };
}

