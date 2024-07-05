/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
//#include "BasicMultiPassrenderer->h"
//#include "../nclgl/Win32Window.h"
//
//#include "../Plugins/VulkanRendering/VulkanMesh.h"
//#include "../Plugins/VulkanRendering/VulkanPipelineBuilder.h"
//
//
//using namespace NCL;
//using namespace Rendering;
//
///*
//BasicMultiPassRenderer::BasicMultiPassRenderer(Window& window, VulkanInitialisation& vkInit) : VulkanRenderer(window)
//{
//	camera = Camera::BuildPerspectiveCamera(Vector3(0, 0, 1), 0, 0, 45.0f, 0.1f, 100.0f);
//
//	BuildBasicDescriptorPool();
//
//	triangleMesh	= VulkanMesh::GenerateTriangle(this);
//	quadMesh		= VulkanMesh::GenerateTriangle(this);
//
//	sceneShader		= VulkanShader::CreateShaderFromGLSL("BasicUniformBuffer.vert", "BasicUniformBuffer.frag", device);
//	screenShader	= VulkanShader::CreateShaderFromGLSL("BasicUniformBuffer.vert", "BasicUniformBuffer.frag", device);
//
//	CreateSceneDrawPipeline();
//	CreateSceneScreenPipeline();
//
//	Matrix4 camMatrices[2];
//	WriteUniform(cameraData, (void*)&camMatrices, sizeof(Matrix4) * 2);
//	cameraMemory = (Matrix4*)device.mapMemory(cameraData.deviceMem, 0, cameraData.allocInfo.allocationSize);
//}
//
//
//BasicMultiPassRenderer::~BasicMultiPassRenderer()
//{
//}
//
//void BasicMultiPassRenderer::Update(float msec) {
//	camera.UpdateCamera(msec);
//}
//
//void BasicMultiPassRenderer::Initialise() {
//	InitPipeline(basicPipeline);	//Only needs doing once
//}
//
////We must intercept resize calls so we can recreate the off screen buffers...
//void BasicMultiPassRenderer::Resize(int width, int height) {
//	VulkanRenderer::Resize(width, height);
//}
//
//void	BasicMultiPassRenderer::CreateSceneDrawPipeline() {
//	//For this we need a screen sized colour buffer and depth buffer
//	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();
//	pipeLayoutCreate.setSetLayoutCount(sceneDrawPipe.layouts.size());
//	pipeLayoutCreate.setPSetLayouts(&sceneDrawPipe.layouts[0]);
//
//
//	sceneDrawPipe = VulkanPipelineBuilder()
//		.WithMeshState(triangleMesh->GetVertexBuffer()->vertexInfo)	
//		.WithLayout(device.createPipelineLayout(pipeLayoutCreate))
//		.Build(device, pipelineCache);
//}
//
//void	BasicMultiPassRenderer::CreateSceneScreenPipeline() {
//	//For this one we just need the default back buffer
//	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();
//	pipeLayoutCreate.setSetLayoutCount(sceneScreenPipe.layouts.size());
//	pipeLayoutCreate.setPSetLayouts(&sceneScreenPipe.layouts[0]);
//
//	sceneScreenPipe = VulkanPipelineBuilder()
//		.WithMeshState(quadMesh->GetVertexBuffer()->vertexInfo)
//		.Build(device, pipelineCache);
//}
//
//
//void BasicMultiPassRenderer::FinishPipeline(vk::GraphicsPipelineCreateInfo& pipelineCreate, VulkanPipeline& pipeline) {
//	//triangleMesh			= VulkanMesh::GenerateTriangle(this);
//	//basicPipeline.shader	= VulkanShader::CreateShaderFromGLSL("BasicUniformBuffer.vert", "BasicUniformBuffer.frag", device);
//
//	//pipelineCreate.setStageCount(basicPipeline.shader->GetNumSubStages());
//	//pipelineCreate.setPVertexInputState(&triangleMesh->GetVertexBuffer()->vertexInfo);
//
//	//vk::PipelineShaderStageCreateInfo* shaderCreate = new vk::PipelineShaderStageCreateInfo[pipelineCreate.stageCount];
//	//basicPipeline.shader->FillShaderStageCreateInfo(shaderCreate, pipelineCreate.stageCount);
//	//pipelineCreate.setPStages(shaderCreate);
//
//
//
//	BuildBasicDescriptorSetLayout(pipeline);
//
//	BuildBasicDescriptorSet(pipeline);
//
//	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();
//	pipeLayoutCreate.setSetLayoutCount(pipeline.layouts.size());
//	pipeLayoutCreate.setPSetLayouts(&pipeline.layouts[0]);
//
//	basicPipeline.pipelineLayout = device.createPipelineLayout(pipeLayoutCreate);
//
//
//}
//
//void BasicMultiPassRenderer::RenderFrame() {
//	frameCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, basicPipeline);
//
//	UpdateCameraUniform();
//
//	frameCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, basicPipeline.pipelineLayout, 0, 1, &defaultDescriptorSet, 0, nullptr);
//
//	triangleMesh->SubmitDraw(frameCmdBuffer);
//}
//
//void	BasicMultiPassRenderer::BuildBasicDescriptorSetLayout(VulkanPipeline& pipeline) {
//	vk::DescriptorSetLayoutBinding bindings[1];
//
//	bindings[0].binding				= 0;
//	bindings[0].descriptorCount		= 1;
//	bindings[0].descriptorType		= vk::DescriptorType::eUniformBuffer;
//	bindings[0].pImmutableSamplers	= NULL;
//	bindings[0].stageFlags			= vk::ShaderStageFlagBits::eVertex;
//
//	vk::DescriptorSetLayoutCreateInfo descriptorCreate;
//	descriptorCreate.bindingCount	= sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
//	descriptorCreate.pBindings		= bindings;
//
//	pipeline.layouts.push_back(device.createDescriptorSetLayout(descriptorCreate));
//}
//
//void	BasicMultiPassRenderer::BuildBasicDescriptorPool() {
//	vk::DescriptorPoolSize poolSizes[1];
//
//	poolSizes[0].type			 = vk::DescriptorType::eUniformBuffer;
//	poolSizes[0].descriptorCount = 1;
//
//	vk::DescriptorPoolCreateInfo poolCreate;
//	poolCreate.setPoolSizeCount(1);
//	poolCreate.setPPoolSizes(poolSizes);
//	poolCreate.setMaxSets(1);
//
//	defaultDescriptorPool = device.createDescriptorPool(poolCreate);
//}
//
//void	BasicMultiPassRenderer::BuildBasicDescriptorSet(VulkanPipeline& pipeline) {
//	vk::DescriptorSetAllocateInfo allocateInfo;
//
//	allocateInfo.setDescriptorPool(defaultDescriptorPool);
//	allocateInfo.setDescriptorSetCount(pipeline.layouts.size());
//	allocateInfo.setPSetLayouts(&pipeline.layouts[0]);
//
//	device.allocateDescriptorSets(&allocateInfo, &defaultDescriptorSet);
//
//	vk::WriteDescriptorSet descriptorWrites[1];
//
//	descriptorWrites[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
//	descriptorWrites[0].setDstSet(defaultDescriptorSet);
//	descriptorWrites[0].setDstBinding(0);
//	descriptorWrites[0].setDescriptorCount(1);
//	descriptorWrites[0].setPBufferInfo(&cameraData.descriptorInfo);
//
//	device.updateDescriptorSets(1, descriptorWrites, 0, nullptr);
//}
//
//void BasicMultiPassRenderer::UpdateCameraUniform() {
//	Matrix4 camMatrices[2];
//
//	float currentAspect = (float)currentWidth / (float)currentHeight;
//
//	camMatrices[0] = camera.BuildViewMatrix();
//	camMatrices[1] = camera.BuildProjectionMatrix(currentAspect);
//
//	//'Traditional' method...
//	//UpdateUniform(cameraData, (void*)&camMatrices, sizeof(Matrix4) * 2);
//
//	//'Modern' method - just permamap the buffer and transfer the new data directly to GPU
//	//Only works with the host visible bit set...
//	cameraMemory[0] = camera.BuildViewMatrix();
//	cameraMemory[1] = camera.BuildProjectionMatrix(currentAspect);
//}
//
//*/
