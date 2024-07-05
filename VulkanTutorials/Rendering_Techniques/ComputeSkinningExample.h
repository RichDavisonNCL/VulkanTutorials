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
    class ComputeSkinningExample : public VulkanTutorial
    {
    public:
        ComputeSkinningExample(Window& window, VulkanInitialisation& vkInit);
        ~ComputeSkinningExample();

    protected:
        void RenderFrame(float dt) override;

        UniqueVulkanShader  drawShader;
        UniqueVulkanCompute skinShader;

        GLTFScene scene;

        VulkanBuffer		            jointsBuffer;
        VulkanBuffer		            outputVertices;

        vk::UniqueDescriptorSetLayout	textureLayout;
        std::vector<std::vector<vk::UniqueDescriptorSet>>	 layerDescriptors;

        std::vector < vk::UniqueDescriptorSet > layerSets;

        vk::UniqueDescriptorSet		    computeDescriptor;
        vk::UniqueDescriptorSetLayout   computeLayout;

        vk::UniqueSemaphore computeSemaphore;

        VulkanPipeline		drawPipeline;
        VulkanPipeline		computePipeline;

        vk::UniqueCommandBuffer asyncCmds;
        vk::UniqueCommandBuffer renderCmds;

        float   frameTime       = 0.0f;
        int     currentFrame    = 0;
    };
}

