/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanBVHBuilder.h"
#include "VulkanRTShader.h"
#include "../VulkanTutorials/VulkanTutorial.h"
#include "VulkanShaderBindingTableBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"

namespace NCL::Rendering::Vulkan {
	using UniqueVulkanRTShader = std::unique_ptr<VulkanRTShader>;
	using SharedVulkanRTShader = std::shared_ptr<VulkanRTShader>;

	class TestGLTFRayTrace : public VulkanTutorial	{
	public:
		TestGLTFRayTrace(Window& window, VulkanInitialisation& vkInit);
		~TestGLTFRayTrace();

	protected:
		void RenderFrame(float dt) override;

		GLTFScene scene;

		VulkanPipeline		displayPipeline;
		UniqueVulkanShader	displayShader;

		VulkanPipeline		rtPipeline;
		UniqueVulkanMesh	quadMesh;

		vk::UniqueDescriptorSetLayout	rayTraceLayout;
		vk::UniqueDescriptorSet			rayTraceDescriptor;

		vk::UniqueDescriptorSetLayout	imageLayout;
		vk::UniqueDescriptorSet			imageDescriptor;

		vk::UniqueDescriptorSetLayout	inverseCamLayout;
		vk::UniqueDescriptorSet			inverseCamDescriptor;
		VulkanBuffer					inverseMatrices;

		vk::UniqueDescriptorSetLayout	displayImageLayout;
		vk::UniqueDescriptorSet			displayImageDescriptor;

		UniqueVulkanTexture				rayTexture;
		vk::ImageView					imageWriteView;

		ShaderBindingTable				bindingTable;
		VulkanBVHBuilder				bvhBuilder;		
		vk::UniqueAccelerationStructureKHR	tlas;

		UniqueVulkanRTShader	raygenShader;
		UniqueVulkanRTShader	hitShader;
		UniqueVulkanRTShader	missShader;
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR	rayPipelineProperties;
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR	rayAccelFeatures;
	};
}

