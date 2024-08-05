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

	struct SceneNodeDes
	{
		uint32_t nodeID = 0;
		std::vector<uint32_t> matLayerId;

		SceneNodeDes(uint32_t inMeshId, const std::vector<uint32_t>& inMatLayerId)
		{
			nodeID = inMeshId;
			matLayerId = inMatLayerId;
		}

		SceneNodeDes(uint32_t inMeshId)
		{
			nodeID = inMeshId;
		}
	};

	struct MaterialLayer 
	{
		int albedoId{ -1 };
		int bumpId{ -1 };
		int occlusionId{ -1 };
		int emissionId{ -1 };
		int metallicId{ -1 };

		MaterialLayer(const GLTFMaterialLayer& inMatLayer)
		{
			albedoId = inMatLayer.albedoId;
			bumpId = inMatLayer.bumpId;
			occlusionId = inMatLayer.occlusionId;
			emissionId = inMatLayer.emissionId;
			metallicId = inMatLayer.metallicId;
		}
	};

	class TestGLTFRayTrace : public VulkanTutorial	{
	public:
		TestGLTFRayTrace(Window& window);
		~TestGLTFRayTrace();

	protected:
		void BuildTlas(const vk::Device& device);
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

		//----------------------------------------------------------------------------------------------------
		UniqueVulkanTexture	defaultTexture;
		vk::UniqueDescriptorSetLayout	defaultSamplerLayout;
		vk::UniqueDescriptorSet		  defaultSamplerDescriptor;

		vk::UniqueDescriptorSetLayout	dSamplerLayout;
		vk::UniqueDescriptorSet		  dSamplerDescriptor;

		UniqueVulkanRTShader	hitShader2;

		vk::UniqueDescriptorSetLayout rtSceneBufferLayout;
		vk::UniqueDescriptorSet			rtSceneBufferDescriptor;
		VulkanBuffer	texCordBuffer;
		VulkanBuffer	vertexPositionBuffer;
		VulkanBuffer	vertexNormalBuffer;
		VulkanBuffer	indicesBuffer;
		/// <summary>
		/// Buffer to store texturemap and material layer map in a big array
		/// </summary>
		VulkanBuffer	textureMapBuffer;

		VulkanBuffer	matLayerBuffer;
		void BuildVertexBuffer(vk::UniqueDescriptorSetLayout& inDesSetLayout, vk::UniqueDescriptorSet& outDesSet, vk::Device& device, vk::DescriptorPool& pool);


		VulkanBuffer	sceneDesBuffer;

		void SceneDesBufferBuild(vk::Device& device, vk::DescriptorPool& pool);
		//--------------------------------------------------------------------------------------------------
	};
}