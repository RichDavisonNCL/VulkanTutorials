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
        static void InitBindlessDevice(vk::DeviceCreateInfo&, vk::PhysicalDevice device);

    protected:
        void CreateBindlessDescriptorPool();
        void UpdateBindlessImageDescriptor(vk::DescriptorSet set, int bindingSlot, int index, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout);

        void SetupDeviceInfo(vk::DeviceCreateInfo& info) override;

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
