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

SkinningExample::SkinningExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)
{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);

	m_camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		UploadMeshWait(*loadedMesh);
	}

	textureLayout = DescriptorSetLayoutBuilder(context.device)
		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
		.Build("Object Textures");

	for (const auto& m : scene.meshMaterials) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.layers) {
			GLTFMaterial& layer = scene.materials[l];
			matSet.push_back(CreateDescriptorSet(context.device, context.descriptorPool, *textureLayout));
			WriteCombinedImageDescriptor(context.device, *matSet.back(), 0,((VulkanTexture*)layer.albedo.get())->GetDefaultView(), *m_defaultSampler);
		}
	}

	jointsLayout = DescriptorSetLayoutBuilder(context.device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("Joint Data"); //Get our m_camera matrices...

	jointsDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, *jointsLayout);

	VulkanMesh* m = (VulkanMesh*)scene.meshes[0].get();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("BasicSkinning.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("SingleTexture.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("Main Scene Pipeline");

	int matCount = scene.meshes[0]->GetJointCount();

	jointsBuffer = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Matrix4) * matCount,
			.usage	= vk::BufferUsageFlagBits::eStorageBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Joint Matrices"
	);


	std::vector<Matrix4> bindPose	= scene.meshes[0]->GetBindPose();
	std::vector<Matrix4> invBindPos	= scene.meshes[0]->GetInverseBindPose();

	std::vector<Matrix4> testData;

	for (size_t i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	jointsBuffer.CopyData(testData.data(), sizeof(Matrix4) * matCount);

	WriteBufferDescriptor(context.device,*jointsDescriptor, 0, vk::DescriptorType::eStorageBuffer, jointsBuffer);
	WriteBufferDescriptor(context.device,*m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
}

void SkinningExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer m_cmdBuffer = context.cmdBuffer;

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

	m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);
	m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*jointsDescriptor, 0, nullptr);

	for (size_t i = 0; i < scene.meshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)scene.meshes[i].get();
		std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			m_cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);

			loadedMesh->DrawLayer( j, m_cmdBuffer);
		}
	}
}