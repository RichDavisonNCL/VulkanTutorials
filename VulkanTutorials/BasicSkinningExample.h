/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"
#include "../GLTFLoader/GLTFLoader.h"

namespace NCL::Rendering::Vulkan {
    class BasicSkinningExample : public VulkanTutorialRenderer
    {
    public:
        BasicSkinningExample(Window& window);
        ~BasicSkinningExample() {}

        void SetupTutorial() override;

        void RenderFrame()		override;
        void Update(float dt)	override;
    protected:
        UniqueVulkanShader shader;
        GLTFLoader loader;

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

