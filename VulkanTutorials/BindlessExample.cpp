/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BindlessExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

const int _NumSamplers = 1024;

BindlessExample::BindlessExample(Window& window) : VulkanTutorialRenderer(window){
}

void BindlessExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	camera.SetYaw(270.0f).SetPitch(-50.0f).SetPosition({-10, 24, 15});
	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	cubeMesh = LoadMesh("Cube.msh");

	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BindlessExample.vert.spv")
		.WithFragmentBinary("BindlessExample.frag.spv")
		.Build("Bindless Example Shader!");

	CreateBindlessDescriptorPool();
	descriptorLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build("Matrix Data");

	bindlessLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(_NumSamplers, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(_NumSamplers, vk::ShaderStageFlagBits::eFragment, vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		.Build("Bindless Data");

	pipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0, *cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(1, *descriptorLayout)	//All matrices are 1
		.WithDescriptorSetLayout(2, *bindlessLayout)	//All textures Set 2
		.Build("Bindless Pipeline");

	descriptorSet	= BuildUniqueDescriptorSet(*descriptorLayout);
	bindlessSet		= BuildUniqueDescriptorSet(*bindlessLayout, *bindlessDescriptorPool, _NumSamplers);

	matrices = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * _NumSamplers, "Matrices");

	Matrix4* mappedData = (Matrix4*)matrices.Data();
	int axis = (int)sqrt(_NumSamplers);
	for (int i = 0; i < _NumSamplers; ++i) {
		mappedData[i] = Matrix4::Translation({float(i / axis), 0, float(i % axis)});
		WriteImageDescriptor(*bindlessSet, 1, i, *textures[rand() % 2], *defaultSampler);
	}

	WriteBufferDescriptor(*descriptorSet, 0, vk::DescriptorType::eStorageBuffer, matrices);
}

void BindlessExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = true;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
	indexingFeatures.descriptorBindingPartiallyBound = true;
	indexingFeatures.descriptorBindingVariableDescriptorCount = true;
	indexingFeatures.runtimeDescriptorArray = true;
	deviceFeatures.setPNext((void*)&indexingFeatures);

	deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
}

void BindlessExample::CreateBindlessDescriptorPool() {
	vk::DescriptorPoolSize poolSizes[] = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, _NumSamplers),
	};

	vk::DescriptorPoolCreateInfo poolCreate;
	poolCreate.setPoolSizeCount(sizeof(poolSizes) / sizeof(vk::DescriptorPoolSize));
	poolCreate.setPPoolSizes(poolSizes);
	poolCreate.setMaxSets(128);//how many times can we ask the pool for a descriptor set?
	poolCreate.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	bindlessDescriptorPool = GetDevice().createDescriptorPoolUnique(poolCreate);
}

void BindlessExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorSet sets[3] = {
		*cameraDescriptor,
		*descriptorSet,
		*bindlessSet
	};

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 3, sets, 0, nullptr);

	DrawMesh(frameCmds, *cubeMesh, _NumSamplers);
}