///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
#include "BindlessExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(BindlessExample)

const int _NumSamplers = 1024;

//We need update after bind to allow for the shader to access more samplers than can be bound by default

BindlessExample::BindlessExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit){
	static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = true;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
	indexingFeatures.descriptorBindingPartiallyBound = true;
	indexingFeatures.descriptorBindingVariableDescriptorCount = true;
	indexingFeatures.runtimeDescriptorArray = true;

	m_vkInit.features.push_back(&indexingFeatures);

	m_vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	m_vkInit.defaultDescriptorPoolFlags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT;
	m_vkInit.defaultDescriptorPoolImageCount = _NumSamplers;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	m_camera.SetYaw(270.0f).SetPitch(-50.0f).SetPosition({-10, 24, 15});
	textures[0] = LoadTexture("Vulkan.png");
	textures[1] = LoadTexture("Doge.png");

	cubeMesh = LoadMesh("Cube.msh");

	bindlessLayout = DescriptorSetLayoutBuilder(context.device)
		.WithImageSamplers(0, _NumSamplers, vk::ShaderStageFlagBits::eFragment, vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount)
		.WithCreationFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		.Build("Bindless Data");

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.WithShaderBinary("BindlessExample.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BindlessExample.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDescriptorSetLayout(2, *bindlessLayout)	//All textures Set 2
		.Build("Bindless Pipeline");

	descriptorSet	= CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(1));
	bindlessSet		= CreateDescriptorSet(context.device, context.descriptorPool, *bindlessLayout, _NumSamplers);

	matrices = m_memoryManager->CreateBuffer(
		{
				.size = sizeof(Matrix4) * _NumSamplers,
				.usage = vk::BufferUsageFlagBits::eStorageBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		"Matrices"
	);

	Matrix4* mappedData = matrices.Map<Matrix4>();
	int axis = (int)sqrt(_NumSamplers);
	for (int i = 0; i < _NumSamplers; ++i) {
		mappedData[i] = Matrix::Translation(Vector3{float(i / axis), 0, float(i % axis)});
		WriteCombinedImageDescriptor(context.device , *bindlessSet, 0, i, *textures[rand() % 2], *m_defaultSampler);
	}
	matrices.Unmap();

	WriteBufferDescriptor(context.device , *descriptorSet, 0, vk::DescriptorType::eStorageBuffer, matrices);
}

void BindlessExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::DescriptorSet sets[3] = {
		*m_cameraDescriptor,
		*descriptorSet,
		*bindlessSet
	};

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, std::size(sets), sets, 0, nullptr);

	cubeMesh->Draw(context.cmdBuffer, _NumSamplers);
}