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
        ~MultiViewportExample() {}

        void SetupTutorial() override;

    protected:
        virtual void RenderFrame();

        UniqueVulkanMesh    triMesh;
        UniqueVulkanShader  shader;
        VulkanPipeline	    pipeline;
    };
}

