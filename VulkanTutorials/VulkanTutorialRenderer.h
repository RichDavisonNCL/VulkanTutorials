/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/TextureLoader.h"

#include "../VulkanRendering/VulkanUtils.h"

namespace NCL::Rendering {
	struct CameraUniform {
		Camera			camera;
		VulkanBuffer	cameraData;
		Matrix4*		cameraMemory;
	};

	struct RenderObject {
		VulkanMesh* mesh;
		Matrix4 transform;
		vk::UniqueDescriptorSet objectDescriptorSet;
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

	class VulkanTutorialRenderer : public VulkanRenderer	{
	public:
		VulkanTutorialRenderer(Window& window);
		~VulkanTutorialRenderer();

		virtual void SetupTutorial();

		virtual void Update(float dt) {
			runTime += dt;
			UpdateCamera(dt);
			UploadCameraUniform();
		}

	protected:

		void BuildCamera();
		void UpdateCamera(float dt);
		void UploadCameraUniform();

		void RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet = 0);

		UniqueVulkanMesh LoadMesh(const string& filename);

		UniqueVulkanMesh GenerateTriangle();
		UniqueVulkanMesh GenerateQuad();
		UniqueVulkanMesh GenerateGrid();

		CameraUniform cameraUniform;
		vk::UniqueDescriptorSetLayout nullLayout;

		vk::UniqueDescriptorSet			cameraDescriptor;
		vk::UniqueDescriptorSetLayout	cameraLayout;

		vk::UniqueSampler		defaultSampler;

		float runTime;
	};
}