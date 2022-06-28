/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/TextureLoader.h"

#include "../../Plugins/VulkanRendering/VulkanUtils.h"

namespace NCL::Rendering {
	struct CameraUniform {
		Camera			camera;
		VulkanBuffer	cameraData;
		Matrix4*		cameraMemory;
	};

	struct VulkanFrameBuffer {
		vk::UniqueFramebuffer frameBuffer;
		int width;
		int height;
		vector<std::shared_ptr<VulkanTexture>> colourAttachments;
		std::shared_ptr<VulkanTexture> depthAttachment;
		//vk::RenderPass createdPass;
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
		VulkanTutorialRenderer(Window& window, VulkanInitInfo info = VulkanInitInfo());
		~VulkanTutorialRenderer();

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

		VulkanFrameBuffer BuildFrameBuffer(vk::RenderPass& pass, int width, int height, uint32_t colourCount, bool hasDepth, bool floatTex = false, bool stencilDepth = true);

		VulkanFrameBuffer BuildFrameBuffer(vk::RenderPass& pass, int width, int height, vector<std::shared_ptr<VulkanTexture>> colourAttachments, std::shared_ptr<VulkanTexture> depthAttacment = std::shared_ptr<VulkanTexture>());

		vk::RenderPass BuildPassForFrameBuffer(VulkanFrameBuffer& buffer, bool clearColours = true, bool clearDepth = true);

		void TransitionColourToSampler(VulkanTexture*t, vk::CommandBuffer  buffer);
		void TransitionDepthToSampler(VulkanTexture* t, vk::CommandBuffer  buffer, bool doStencil = false);

		void TransitionSamplerToColour(VulkanTexture* t, vk::CommandBuffer  buffer);
		void TransitionSamplerToDepth(VulkanTexture* t, vk::CommandBuffer  buffer, bool doStencil = false);

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