/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "LightingExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

LightingExample::LightingExample(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	vkInit.autoBeginDynamicRendering = true;
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	camera.SetPitch(-20.0f).SetPosition({ 0, 100.0f, 200 });

	cubeMesh = LoadMesh("Cube.msh");

	boxObject.mesh		= &*cubeMesh;
	boxObject.transform = Matrix::Translation(Vector3{ -20, 10, -20 }) * Matrix::Scale(Vector3{ 10.0f, 10.0f, 10.0f });

	floorObject.mesh		= &*cubeMesh;
	floorObject.transform = Matrix::Scale(Vector3{ 100.0f, 1.0f, 100.0f });

	lightingShader = ShaderBuilder(renderer->GetDevice())
		.WithVertexBinary("Lighting.vert.spv")
		.WithFragmentBinary("Lighting.frag.spv")
	.Build("Lighting Shader");

	Light testLight(Vector3(0, 20, -30), 150.0f, Vector4(1, 0.8f, 0.5f, 1));

	lightUniform = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Light), "Light Uniform");

	camPosUniform = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.Build(sizeof(Vector3), "Camera Position Uniform");

	lightUniform.CopyData((void*)&testLight, sizeof(testLight));

	allTextures[0] = LoadTexture("rust_diffuse.png");
	allTextures[1] = LoadTexture("rust_bump.png");
	allTextures[2] = LoadTexture("concrete_diffuse.png");
	allTextures[3] = LoadTexture("concrete_bump.png");

	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(lightingShader)
		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.Build("Lighting Pipeline");

	lightDescriptor = CreateDescriptorSet(device, pool, lightingShader->GetLayout(1));
	boxObject.descriptorSet = CreateDescriptorSet(device, pool, lightingShader->GetLayout(2));
	floorObject.descriptorSet = CreateDescriptorSet(device, pool, lightingShader->GetLayout(2));
	cameraPosDescriptor = CreateDescriptorSet(device, pool, lightingShader->GetLayout(3));

	WriteImageDescriptor(device, *boxObject.descriptorSet, 0, allTextures[0]->GetDefaultView(), *defaultSampler);
	WriteImageDescriptor(device, *boxObject.descriptorSet, 1, allTextures[1]->GetDefaultView(), *defaultSampler);

	WriteImageDescriptor(device, *floorObject.descriptorSet, 0, allTextures[2]->GetDefaultView(), *defaultSampler);
	WriteImageDescriptor(device, *floorObject.descriptorSet, 1, allTextures[3]->GetDefaultView(), *defaultSampler);

	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	WriteBufferDescriptor(device, *lightDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightUniform);
	WriteBufferDescriptor(device, *cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);
}

void LightingExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = state.cmdBuffer;

	Vector3 newCamPos = camera.GetPosition();
	camPosUniform.CopyData((void*)&newCamPos, sizeof(Vector3));

	vk::Device device = renderer->GetDevice();





	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	WriteBufferDescriptor(device, *lightDescriptor, 0, vk::DescriptorType::eUniformBuffer, lightUniform);
	WriteBufferDescriptor(device, *cameraPosDescriptor, 0, vk::DescriptorType::eUniformBuffer, camPosUniform);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*lightDescriptor, 0, nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 3, 1, &*cameraPosDescriptor, 0, nullptr);

	RenderSingleObject(boxObject	, cmdBuffer, pipeline, 2);
	RenderSingleObject(floorObject	, cmdBuffer, pipeline, 2);
}