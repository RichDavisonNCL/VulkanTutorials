/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanTutorialRenderer.h"
#include "GLTFLoader.h"

namespace NCL::Rendering {
    class BasicSkinningExample : public VulkanTutorialRenderer
    {
    public:
        BasicSkinningExample(Window& window);
        ~BasicSkinningExample() {}

        void RenderFrame()		override;
        void Update(float dt)	override;
    protected:
        UniqueVulkanShader shader;
        GLTFLoader loader;

        VulkanBuffer		    jointsBuffer;
        vk::DescriptorSet		jointsDescriptor;
        vk::UniqueDescriptorSetLayout jointsLayout;

        vk::UniqueDescriptorSetLayout	textureLayout;
        vector<vector<vk::UniqueDescriptorSet>>	 layerDescriptors;

        vector < vk::UniqueDescriptorSet > layerSets;

        VulkanPipeline		pipeline;

        float   frameTime;
        int     currentFrame;
    };
}

