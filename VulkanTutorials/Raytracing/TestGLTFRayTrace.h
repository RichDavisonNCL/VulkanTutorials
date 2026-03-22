/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanBVHBuilder.h"
#include "../VulkanTutorials/VulkanTutorial.h"
#include "VulkanShaderBindingTableBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"

#if USE_IMGUI
#include "Gui.h"
#endif // USE_IMGUI

namespace NCL::Rendering::Vulkan {
	enum class PathTracerDebugOutputType : uint32_t
	{
		None = 0,
		DiffuseReflectance = 1,
		WorldSpaceNormals = 2,
		WorldSpacePosition = 3,
		Barycentrics = 4,
		HitT = 5,
		InstanceID = 6,
		Emissives = 7,
		BounceHeatmap = 8,
	};

	class TestGLTFRayTrace : public VulkanTutorial	{
	public:
		TestGLTFRayTrace(Window& window, VulkanInitialisation& vkInit);
		~TestGLTFRayTrace();

	protected:
		void RenderFrame(float dt) override;
		void Update(float dt) override;

		GLTFScene scene;

		VulkanPipeline		displayPipeline;

		VulkanPipeline		rtPipeline;

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

		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR	rayPipelineProperties;
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR	rayAccelFeatures;

#if USE_IMGUI
		Gui							ui;

		PathTracerDebugOutputType	ptDebugOutput = PathTracerDebugOutputType::None;
		const char*					ptDebugOutputTypeStrings = "None\0Diffuse Reflectance\0Worldspace Normals\0Worldspace Position\0Barycentrics\0HitT\0InstanceID\0Emissives\0Bounce Heatmap\0";
		VulkanBuffer				ptDebugOptionsBuffer;

#endif //USE_IMGUI
	};
}

