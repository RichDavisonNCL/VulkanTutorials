/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "SkinningExample.h"
#include "../NCLCoreClasses/MeshAnimation.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(SkinningExample)

SkinningExample::SkinningExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window)
{
	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& frameState = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);

	camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		loadedMesh->UploadToGPU(renderer);
	}

	textureLayout = DescriptorSetLayoutBuilder(device)
		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
		.Build("Object Textures");

	for (const auto& m : scene.materials) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(CreateDescriptorSet(device, pool, *textureLayout));
			WriteImageDescriptor(device, *matSet.back(), 0,((VulkanTexture*)l.albedo.get())->GetDefaultView(), *defaultSampler);
		}
	}
	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicSkinning.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	jointsLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("Joint Data"); //Get our camera matrices...

	jointsDescriptor = CreateDescriptorSet(device, pool, *jointsLayout);

	VulkanMesh* m = (VulkanMesh*)scene.meshes[0].get();

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("Main Scene Pipeline");

	int matCount = scene.meshes[0]->GetJointCount();

	jointsBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * matCount, "Joint Matrices");

	std::vector<Matrix4> bindPose	= scene.meshes[0]->GetBindPose();
	std::vector<Matrix4> invBindPos	= scene.meshes[0]->GetInverseBindPose();

	std::vector<Matrix4> testData;

	for (size_t i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	jointsBuffer.CopyData(testData.data(), sizeof(Matrix4) * matCount);

	WriteBufferDescriptor(device,*jointsDescriptor, 0, vk::DescriptorType::eStorageBuffer, jointsBuffer);
	WriteBufferDescriptor(device,*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
}

void SkinningExample::RenderFrame(float dt) {
	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	frameTime -= dt;

	Mesh*  mesh = scene.meshes[0].get();
	MeshAnimation* anim = scene.animations[0].get();

	if (frameTime <= 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += anim->GetFrameTime();
		std::vector<Matrix4> invBindPos = mesh->GetInverseBindPose();

		const Matrix4* frameMats = anim->GetJointData(currentFrame);

		std::vector<Matrix4> jointData(invBindPos.size());

		for (int i = 0; i < invBindPos.size(); ++i) {
			jointData[i] = (frameMats[i] * invBindPos[i]);
		}
		jointsBuffer.CopyData(jointData.data(), sizeof(Matrix4) * jointData.size());
	}

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*jointsDescriptor, 0, nullptr);

	for (size_t i = 0; i < scene.meshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)scene.meshes[i].get();
		std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);

			loadedMesh->DrawLayer( j, cmdBuffer);
		}
	}
}