/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
    class BufferDeviceAddressExample : public VulkanTutorial   {
    public:
        BufferDeviceAddressExample(Window& window, VulkanInitialisation& vkInit);
        ~BufferDeviceAddressExample() {}

    protected:
        void RenderFrame(float dt) override;

        VulkanPipeline	    pipeline;

        UniqueVulkanMesh 	triMesh;
        UniqueVulkanShader	shader;

        VulkanBuffer	    pointerBuffer;

        VulkanBuffer	    dataBuffers[3];
    };
}
