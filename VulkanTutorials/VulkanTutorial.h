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
		VulkanMesh*				mesh;
		Matrix4					transform;
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
		VulkanTutorial(Window& window, VulkanInitialisation& vkInit);
		virtual ~VulkanTutorial();

		virtual void Update(float dt) {
			m_runTime += dt;
			UpdateCamera(dt);
			UploadCameraUniform();

			m_renderer->Update(dt);
		}

		virtual void RunFrame(float dt);

		void Finish() const;

		void WindowEventHandler(WindowEvent e, uint32_t w, uint32_t h);

		static VulkanTutorial*		CreateTutorial(int& chainID, VulkanInitialisation& vkInit);
		static VulkanTutorial*		CreateTutorial(const std::string& name, VulkanInitialisation& vkInit);
		static VulkanInitialisation	DefaultInitialisation();

	protected:
		virtual void RenderFrame(float dt) = 0;
		virtual void OnWindowResize(uint32_t width, uint32_t height) {

		}
		void Initialise();

		void BuildCamera();
		void UpdateCamera(float dt);
		void UploadCameraUniform();

		void RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet = 0);

		UniqueVulkanMesh	LoadMesh(const string& filename, vk::BufferUsageFlags bufferUsage = {});

		void UploadMeshWait(VulkanMesh& m, vk::BufferUsageFlags bufferUsage = {});
		UniqueVulkanTexture LoadTexture(const string& filename);

		UniqueVulkanTexture LoadCubemap(
			const std::string& negativeXFile, const std::string& positiveXFile,
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,
			const std::string& debugName = "CubeMap");

		UniqueVulkanMesh GenerateTriangle();
		UniqueVulkanMesh GenerateQuad();
		UniqueVulkanMesh GenerateGrid();

		VulkanInitialisation	m_vkInit;
		VulkanRenderer*			m_renderer;
		VulkanMemoryManager*	m_memoryManager;
		NCL::Window&			m_hostWindow;

		KeyboardMouseController m_controller;
		PerspectiveCamera		m_camera;
		VulkanBuffer			m_cameraBuffer;

		vk::UniqueDescriptorSet			m_cameraDescriptor;
		vk::UniqueDescriptorSetLayout	m_cameraLayout;

		vk::UniqueDescriptorSetLayout	m_nullLayout;
		vk::UniqueSampler				m_defaultSampler;

		UniqueVulkanMesh	m_triangleMesh;
		UniqueVulkanMesh	m_quadMesh;
		UniqueVulkanMesh	m_gridMesh;
		UniqueVulkanMesh	m_cubeMesh;
		UniqueVulkanMesh	m_sphereMesh;

		float m_runTime;
	};

#define TUTORIAL_ENTRY(object) static VulkanTutorialEntry entry = VulkanTutorialEntry(#object, [](Window& w, VulkanInitialisation& vk) {return new object(w, vk); });

	using TutorialEntryFunc = std::function < VulkanTutorial*(Window&, VulkanInitialisation&) >;
	class VulkanTutorialEntry {
	public:
		VulkanTutorialEntry(const std::string& s, TutorialEntryFunc f) {
			if (s_listStartPtr) {
				m_nodeChain		= s_listStartPtr;
				s_listStartPtr	= this;
			}
			else {
				s_listStartPtr	= this;
			}
			m_name			= s;
			m_creatorFunc	= f;
		}
		std::string				m_name;
		TutorialEntryFunc		m_creatorFunc;
		VulkanTutorialEntry*	m_nodeChain = nullptr;

		static VulkanTutorialEntry* s_listStartPtr;
	};
}