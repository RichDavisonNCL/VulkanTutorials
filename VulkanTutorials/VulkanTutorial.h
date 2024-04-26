/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/KeyboardMouseController.h"
#include "../NCLCoreClasses/TextureLoader.h"
#include "../VulkanRendering/VulkanUtils.h"

namespace NCL::Rendering::Vulkan {
	struct RenderObject {
		VulkanMesh* mesh;
		Matrix4 transform;
		vk::UniqueDescriptorSet descriptorSet;
	};

	struct Light {
		Vector3 position;
		float	radius;
		Vector4 colour;

		Light() {
			radius = 10.0f;
			colour = Vector4(1, 1, 1, 1);
		}
		Light(const Vector3& inPos, float inRadius, const Vector4& inColour) {
			position	= inPos;
			radius		= inRadius;
			colour		= inColour;
		}
	};

	class VulkanTutorial	{
	public:
		VulkanTutorial(Window& window);
		~VulkanTutorial();

		virtual void Update(float dt) {
			runTime += dt;
			UpdateCamera(dt);
			UploadCameraUniform();

			renderer->Update(dt);
		}

		virtual void RunFrame(float dt);

	protected:
		virtual void RenderFrame(float dt) = 0;

		VulkanInitialisation DefaultInitialisation();

		void InitTutorialObjects();

		void BuildCamera();
		void UpdateCamera(float dt);
		void UploadCameraUniform();

		void RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet = 0);

		UniqueVulkanMesh LoadMesh(const string& filename, vk::BufferUsageFlags bufferUsage = {});
		UniqueVulkanTexture LoadTexture(const string& filename);

		UniqueVulkanTexture LoadCubemap(
			const std::string& negativeXFile, const std::string& positiveXFile,
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,
			const std::string& debugName = "CubeMap");

		UniqueVulkanMesh GenerateTriangle();
		UniqueVulkanMesh GenerateQuad();
		UniqueVulkanMesh GenerateGrid();

		VulkanInitialisation vkInit;
		VulkanRenderer*		renderer;

		PerspectiveCamera	camera;
		VulkanBuffer		cameraBuffer;

		vk::UniqueDescriptorSetLayout nullLayout;

		vk::UniqueDescriptorSet			cameraDescriptor;
		vk::UniqueDescriptorSetLayout	cameraLayout;

		vk::UniqueSampler		defaultSampler;

		KeyboardMouseController controller;

		NCL::Window& hostWindow;

		float runTime;
	};
}