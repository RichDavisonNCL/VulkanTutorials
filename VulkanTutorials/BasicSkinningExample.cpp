/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicSkinningExample.h"
#include "../NCLCoreClasses/MeshAnimation.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

BasicSkinningExample::BasicSkinningExample(Window& window) : VulkanTutorialRenderer(window)
{
}

void BasicSkinningExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	loader.Load("CesiumMan/CesiumMan.gltf",
		[](void) ->  Mesh* {return new VulkanMesh(); },
		[&](std::string& input) ->  VulkanTexture* {return LoadTexture(input).release(); }
	);

	camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	for (const auto& m : loader.outMeshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		loadedMesh->UploadToGPU(this);
	}

	textureLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.Build("Object Textures");

	for (const auto& m : loader.outMats) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(BuildUniqueDescriptorSet(*textureLayout));
			WriteImageDescriptor(*matSet.back(), 0, 0,((VulkanTexture*)l.diffuse.get())->GetDefaultView(), *defaultSampler);
		}
	}
	shader = ShaderBuilder(GetDevice())
		.WithVertexBinary("BasicSkinning.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	jointsLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build("Joint Data"); //Get our camera matrices...

	jointsDescriptor = BuildUniqueDescriptorSet(*jointsLayout);

	VulkanMesh* m = (VulkanMesh*)loader.outMeshes[0].get();

	pipeline = PipelineBuilder(GetDevice())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0,*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)//Textures are set 1
		.WithDescriptorSetLayout(2,*jointsLayout)	//Joints are set 2
	.Build("Main Scene Pipeline");

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * matCount, "Joint Matrices");

	std::vector<Matrix4> bindPose	= loader.outMeshes[0]->GetBindPose();
	std::vector<Matrix4> invBindPos	= loader.outMeshes[0]->GetInverseBindPose();

	std::vector<Matrix4> testData;

	for (size_t i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	jointsBuffer.CopyData(testData.data(), sizeof(Matrix4) * matCount);

	WriteBufferDescriptor(*jointsDescriptor, 0, vk::DescriptorType::eStorageBuffer, jointsBuffer);
	WriteBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
}

void BasicSkinningExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	frameTime -= dt;

	Mesh*  mesh = loader.outMeshes[0].get();
	MeshAnimation* anim = loader.outAnims[0].get();

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
}

void BasicSkinningExample::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*jointsDescriptor, 0, nullptr);

	for (size_t i = 0; i < loader.outMeshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i].get();
		std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);

			DrawMeshLayer(*loadedMesh, j, frameCmds);
		}
	}
}