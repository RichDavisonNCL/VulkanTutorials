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

BindlessExample::BindlessExample(Window& window) : VulkanTutorial(window){
	VulkanInitialisation vkInit = DefaultInitialisation();

	static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = true;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
	indexingFeatures.descriptorBindingPartiallyBound = true;
	indexingFeatures.descriptorBindingVariableDescriptorCount = true;
	indexingFeatures.runtimeDescriptorArray = true;

	vkInit.features.push_back(&indexingFeatures);

	vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& frameState = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	camera.SetYaw(270.0f).SetPitch(-50.0f).SetPosition({-10, 24, 15});
	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	cubeMesh = LoadMesh("Cube.msh");

	shader = ShaderBuilder(device)
		.WithVertexBinary("BindlessExample.vert.spv")
		.WithFragmentBinary("BindlessExample.frag.spv")
		.Build("Bindless Example Shader!");

	CreateBindlessDescriptorPool();

	bindlessLayout = DescriptorSetLayoutBuilder(device)
		//.WithImageSamplers(0, _NumSamplers, vk::ShaderStageFlagBits::eFragment)
		.WithImageSamplers(0, _NumSamplers, vk::ShaderStageFlagBits::eFragment, vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		.Build("Bindless Data");

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.WithShader(shader)
		.WithDescriptorSetLayout(2, *bindlessLayout)	//All textures Set 2
		.Build("Bindless Pipeline");

	descriptorSet	= CreateDescriptorSet(device, pool, shader->GetLayout(1));
	bindlessSet		= CreateDescriptorSet(device, *bindlessDescriptorPool, *bindlessLayout, _NumSamplers);

	matrices = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * _NumSamplers, "Matrices");

	Matrix4* mappedData = (Matrix4*)matrices.Data();
	int axis = (int)sqrt(_NumSamplers);
	for (int i = 0; i < _NumSamplers; ++i) {
		mappedData[i] = Matrix::Translation(Vector3{float(i / axis), 0, float(i % axis)});
		WriteImageDescriptor(device , *bindlessSet, 0, i, *textures[rand() % 2], *defaultSampler);
	}

	WriteBufferDescriptor(device , *descriptorSet, 0, vk::DescriptorType::eStorageBuffer, matrices);
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

	bindlessDescriptorPool = renderer->GetDevice().createDescriptorPoolUnique(poolCreate);
}

void BindlessExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorSet sets[3] = {
		*cameraDescriptor,
		*descriptorSet,
		*bindlessSet
	};

	state.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 3, sets, 0, nullptr);

	cubeMesh->Draw(state.cmdBuffer, _NumSamplers);
}