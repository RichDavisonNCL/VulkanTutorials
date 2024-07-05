/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorial.h"
#include "../GLTFLoader/GLTFLoader.h"

namespace NCL::Rendering::Vulkan {
    class SkinningExample : public VulkanTutorial
    {
    public:
        SkinningExample(Window& window, VulkanInitialisation& vkInit);
        ~SkinningExample() {}

    protected:
        void RenderFrame(float dt);

        UniqueVulkanShader shader;

        GLTFScene  scene;

        VulkanBuffer		            jointsBuffer;
        vk::UniqueDescriptorSet		    jointsDescriptor;
        vk::UniqueDescriptorSetLayout   jointsLayout;

        vk::UniqueDescriptorSetLayout	textureLayout;
        std::vector<std::vector<vk::UniqueDescriptorSet>>	 layerDescriptors;

        std::vector < vk::UniqueDescriptorSet > layerSets;

        VulkanPipeline		pipeline;

        float   frameTime       = 0.0f;
        int     currentFrame    = 0;
    };
}

