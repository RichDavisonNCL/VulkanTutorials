/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../Plugins/VulkanRendering/VulkanRenderer.h"
#include "../Common/Camera.h"
/*
class BasicMultiPassRenderer : public VulkanRenderer
{
public:
	BasicMultiPassRenderer(Window& window);
	~BasicMultiPassRenderer();

	void Initialise() override;
protected:
	void Resize(int width, int height) override;
	void FinishPipeline(vk::GraphicsPipelineCreateInfo& pipelineCreate, VulkanPipeline& pipeline) ;
	void RenderFrame()		override;
	void Update(float msec) override;

	void UpdateCameraUniform();

	void	BuildBasicDescriptorPool();

	void	BuildBasicDescriptorSetLayout(VulkanPipeline& pipeline);
	
	void	BuildBasicDescriptorSet(VulkanPipeline& pipeline);

	void	CreateSceneDrawPipeline();
	void	CreateSceneScreenPipeline();

	VulkanMesh*	triangleMesh;
	VulkanMesh* quadMesh;

	VulkanPipeline			sceneDrawPipe;		//Draws the scene to a FBO;
	VulkanPipeline			sceneScreenPipe;	//Draws the FBO to screen

	vk::RenderPass			sceneDrawPass;
	//vk::RenderPass			processPass;

	VulkanShader*			sceneShader;
	VulkanShader*			screenShader;


	UniformData cameraData;
	Camera		camera;

	Matrix4*	cameraMemory;
};

*/