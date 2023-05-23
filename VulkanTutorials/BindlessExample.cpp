/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BindlessExample.h"

using namespace NCL;
using namespace Rendering;

const int _NumSamplers = 1024;

BindlessExample::BindlessExample(Window& window) : VulkanTutorialRenderer(window/*, bindlessInfo*/)
{

}

void BindlessExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	camera.SetYaw(270.0f).SetPitch(-50.0f).SetPosition({-10, 24, 15});
	textures[0] = VulkanTexture::TextureFromFile(this, "Vulkan.png");
	textures[1] = VulkanTexture::TextureFromFile(this, "Doge.png");

	cubeMesh = LoadMesh("Cube.msh");

	shader = VulkanShaderBuilder("Bindless Example Shader!")
		.WithVertexBinary("BindlessExample.vert.spv")
		.WithFragmentBinary("BindlessExample.frag.spv")
		.Build(GetDevice());

	CreateBindlessDescriptorPool();
	descriptorLayout = VulkanDescriptorSetLayoutBuilder("Matrix Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(GetDevice());

	bindlessLayout = VulkanDescriptorSetLayoutBuilder("Bindless Data")
		.WithSamplers(_NumSamplers, vk::ShaderStageFlagBits::eFragment)
		.WithSamplers(_NumSamplers, vk::ShaderStageFlagBits::eFragment, vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		.Build(GetDevice());

	pipeline = VulkanPipelineBuilder("Bindless Pipeline")
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true)
		.WithDepthFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(1, *descriptorLayout)	//All matrices are 1
		.WithDescriptorSetLayout(2, *bindlessLayout)	//All textures Set 2
		.Build(GetDevice());

	descriptorSet	= BuildUniqueDescriptorSet(*descriptorLayout);
	bindlessSet		= BuildUniqueDescriptorSet(*bindlessLayout, *bindlessDescriptorPool, _NumSamplers);

	matrices = VulkanBufferBuilder(sizeof(Matrix4) * _NumSamplers, "Matrices")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(GetDevice(), GetMemoryAllocator());

	Matrix4* mappedData = (Matrix4*)matrices.Data();
	int axis = (int)sqrt(_NumSamplers);
	for (int i = 0; i < _NumSamplers; ++i) {
		mappedData[i] = Matrix4::Translation({float(i / axis), 0, float(i % axis)});
		UpdateImageDescriptor(*bindlessSet, 1, i, *textures[rand() % 2], *defaultSampler);
	}

	UpdateBufferDescriptor(*descriptorSet, 0, vk::DescriptorType::eStorageBuffer, matrices);
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

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*descriptorSet, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*bindlessSet, 0, nullptr);

	SubmitDrawCall(frameCmds, *cubeMesh, _NumSamplers);
}