/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"

namespace NCL::Rendering {
    class BindlessExample : public VulkanTutorialRenderer   {
    public:
        BindlessExample(Window& window);
        ~BindlessExample() {}

        void SetupTutorial() override;
        void RenderFrame()		override;

    protected:
        void CreateBindlessDescriptorPool();

        void SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) override;

        vk::UniqueDescriptorPool		bindlessDescriptorPool;	//descriptor sets come from here! Specific placement for destructors

        VulkanPipeline	    pipeline;

        UniqueVulkanMesh 	cubeMesh;
        UniqueVulkanShader	shader;

        VulkanBuffer	matrices;

        vk::UniqueDescriptorSet			descriptorSet;
        vk::UniqueDescriptorSetLayout	descriptorLayout;

        vk::UniqueDescriptorSet			bindlessSet;
        vk::UniqueDescriptorSetLayout	bindlessLayout;

        UniqueVulkanTexture	textures[2];
    };
}
