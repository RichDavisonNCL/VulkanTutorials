/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BindlessExample.h"

using namespace NCL;
using namespace Rendering;

VulkanInitInfo bindlessInfo{
	.extensions = {VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME},
	.layers		= {},
	.majorVersion{1},
	.minorVersion{3},
	.deviceModifier{BindlessExample::InitBindlessDevice}
};

void BindlessExample::InitBindlessDevice(vk::DeviceCreateInfo& createInfo, vk::PhysicalDevice gpu) {
	static vk::PhysicalDeviceFeatures2 deviceFeatures;
	static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;

	deviceFeatures.setFeatures(*createInfo.pEnabledFeatures);
	createInfo.pEnabledFeatures = nullptr;

	deviceFeatures.setPNext((void*)&indexingFeatures);

	gpu.getFeatures2(&deviceFeatures, *Vulkan::dispatcher);

	indexingFeatures.pNext = (void*)createInfo.pNext;

	createInfo.setPNext((void*)&deviceFeatures);
}

const int _NumSamplers = 1024;

BindlessExample::BindlessExample(Window& window) : VulkanTutorialRenderer(window, bindlessInfo) {
	cameraUniform.camera.SetYaw(270.0f).SetPitch(-50.0f).SetPosition({-10, 24, 15});
	textures[0] = VulkanTexture::TextureFromFile("Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile("Doge.png");

	cubeMesh = LoadMesh("Cube.msh");

	shader = VulkanShaderBuilder("Bindless Example Shader!")
		.WithVertexBinary("BindlessExample.vert.spv")
		.WithFragmentBinary("BindlessExample.frag.spv")
		.Build(device);

	CreateBindlessDescriptorPool();
	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Matrix Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(device);

	bindlessLayout = VulkanDescriptorSetLayoutBuilder("Bindless Data")
		.WithSamplers(_NumSamplers, vk::ShaderStageFlagBits::eFragment)
		.WithBindlessAccess()
		.Build(device);

	pipeline = VulkanPipelineBuilder()
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(*cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(*descriptorLayout) //All matrices are 1
		.WithDescriptorSetLayout(*bindlessLayout)	//All textures Set 2
		.Build(device, pipelineCache);

	descriptorSet	= BuildUniqueDescriptorSet(*descriptorLayout);
	bindlessSet		= BuildUniqueDescriptorSet(*bindlessLayout, *bindlessDescriptorPool, _NumSamplers);

	matrices = CreateBuffer(sizeof(Matrix4) * _NumSamplers, vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

	Matrix4* mappedData = (Matrix4*)device.mapMemory(*matrices.deviceMem, 0, matrices.allocInfo.allocationSize);
	int axis = (int)sqrt(_NumSamplers);
	for (int i = 0; i < _NumSamplers; ++i) {
		mappedData[i] = Matrix4::Translation({float(i / axis), 0, float(i % axis)});

		UpdateImageDescriptor(*bindlessSet, 0, i, textures[rand() % 2]->GetDefaultView(), *defaultSampler);
	}
	device.unmapMemory(*matrices.deviceMem);

	UpdateBufferDescriptor(*descriptorSet, matrices, 0, vk::DescriptorType::eStorageBuffer);
}

void BindlessExample::CreateBindlessDescriptorPool() {
	int maxSets = 128; //how many times can we ask the pool for a descriptor set?
	vk::DescriptorPoolSize poolSizes[] = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, _NumSamplers),
	};

	vk::DescriptorPoolCreateInfo poolCreate;
	poolCreate.setPoolSizeCount(sizeof(poolSizes) / sizeof(vk::DescriptorPoolSize));
	poolCreate.setPPoolSizes(poolSizes);
	poolCreate.setMaxSets(maxSets);
	poolCreate.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	bindlessDescriptorPool = device.createDescriptorPoolUnique(poolCreate);
}

void BindlessExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);

	VulkanDynamicRenderBuilder()
		.WithColourAttachment(swapChainList[currentSwap]->view)
		.WithDepthAttachment(depthBuffer->GetDefaultView())
		.WithRenderArea(defaultScreenRect)
		.Begin(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	static bool lol = false;

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*descriptorSet, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*bindlessSet, 0, nullptr);

	SubmitDrawCall(*cubeMesh, defaultCmdBuffer, _NumSamplers);

	EndRendering(defaultCmdBuffer);
}