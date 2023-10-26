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
    class ComputeSkinningExample : public VulkanTutorialRenderer
    {
    public:
        ComputeSkinningExample(Window& window);
        ~ComputeSkinningExample();

        void SetupTutorial() override;

        void RenderFrame()		override;
        void Update(float dt)	override;
    protected:
        void SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) override;

        UniqueVulkanShader  drawShader;
        UniqueVulkanCompute skinShader;
        GLTFLoader loader;

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

