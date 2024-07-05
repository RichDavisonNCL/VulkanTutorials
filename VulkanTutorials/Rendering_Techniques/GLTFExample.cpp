/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "GLTFExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(GLTFExample)

GLTFExample::GLTFExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)	{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& frameState = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	GLTFLoader::Load("Sponza/Sponza.gltf",scene);

	camera.SetPitch(-20.0f)
		.SetYaw(90.0f)
		.SetPosition({ 850, 840, -30 })
		.SetFarPlane(5000.0f);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		loadedMesh->UploadToGPU(renderer);
	}

	shader = ShaderBuilder(device)
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	for (const auto& m : scene.materials) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(CreateDescriptorSet(device, pool, shader->GetLayout(1)));
			WriteImageDescriptor(device , *matSet.back(), 0, ((VulkanTexture*)l.albedo.get())->GetDefaultView(), *defaultSampler);
		}
	}

	VulkanMesh* m = (VulkanMesh*)scene.meshes[0].get();
	pipeline = PipelineBuilder(device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("Main Scene Pipeline");
}

void GLTFExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();

	vk::Device device = renderer->GetDevice();
	vk::CommandBuffer cmdBuffer = state.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	Matrix4 identity;
	cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&identity);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
	for(size_t i = 0; i < scene.meshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)scene.meshes[i].get();
		std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);
		
			loadedMesh->DrawLayer(j, cmdBuffer);
		}
	}
}