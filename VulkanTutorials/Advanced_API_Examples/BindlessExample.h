/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"

namespace NCL::Rendering::Vulkan {
    class BindlessExample : public VulkanTutorial   {
    public:
        BindlessExample(Window& window, VulkanInitialisation& vkInit);
        ~BindlessExample() {}

    protected:
        void CreateBindlessDescriptorPool();

        void RenderFrame(float dt) override;

        vk::UniqueDescriptorPool		bindlessDescriptorPool;	//descriptor sets come from here! Specific placement for destructors

        VulkanPipeline	    pipeline;

        UniqueVulkanMesh 	cubeMesh;
        UniqueVulkanShader	shader;

        VulkanBuffer	matrices;

        vk::UniqueDescriptorSet			descriptorSet;


        vk::UniqueDescriptorSet			bindlessSet;
        vk::UniqueDescriptorSetLayout	bindlessLayout;

        UniqueVulkanTexture	textures[2];
    };
}
