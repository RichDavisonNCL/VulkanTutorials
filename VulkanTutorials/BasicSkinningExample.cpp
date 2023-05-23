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

BasicSkinningExample::BasicSkinningExample(Window& window) : VulkanTutorialRenderer(window)
{
}

void BasicSkinningExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	loader.Load("CesiumMan/CesiumMan.gltf",
		[](void) ->  MeshGeometry* {return new VulkanMesh(); },
		[&](std::string& input) ->  VulkanTexture* {return VulkanTexture::TextureFromFile(this, input).release(); }
	);

	camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	for (const auto& m : loader.outMeshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m;
		loadedMesh->UploadToGPU(this);
	}

	textureLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.Build(GetDevice());

	for (const auto& m : loader.outMats) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(BuildUniqueDescriptorSet(*textureLayout));
			UpdateImageDescriptor(*matSet.back(), 0, 0,((VulkanTexture*)l.diffuse)->GetDefaultView(), *defaultSampler);
		}
	}
	shader = VulkanShaderBuilder("Texturing Shader")
		.WithVertexBinary("BasicSkinning.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build(GetDevice());

	jointsLayout = VulkanDescriptorSetLayoutBuilder("Joint Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(GetDevice()); //Get our camera matrices...

	jointsDescriptor = BuildUniqueDescriptorSet(*jointsLayout);

	VulkanMesh* m = (VulkanMesh*)loader.outMeshes[0];

	pipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, false)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithDescriptorSetLayout(0,*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)//Textures are set 1
		.WithDescriptorSetLayout(2,*jointsLayout)	//Joints are set 2
	.Build(GetDevice());

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = VulkanBufferBuilder(sizeof(Matrix4) * matCount, "Joint Matrices")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	vector<Matrix4> bindPose	= loader.outMeshes[0]->GetBindPose();
	vector<Matrix4> invBindPos	= loader.outMeshes[0]->GetInverseBindPose();

	vector<Matrix4> testData;

	for (size_t i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	jointsBuffer.CopyData(testData.data(), sizeof(Matrix4) * matCount);

	UpdateBufferDescriptor(*jointsDescriptor, 0, vk::DescriptorType::eStorageBuffer, jointsBuffer);
	UpdateBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);
}

void BasicSkinningExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	frameTime -= dt;

	MeshGeometry*  mesh = loader.outMeshes[0];
	MeshAnimation* anim = loader.outAnims[0];

	if (frameTime <= 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += anim->GetFrameTime();
		vector<Matrix4> invBindPos = mesh->GetInverseBindPose();

		const Matrix4* frameMats = anim->GetJointData(currentFrame);

		vector<Matrix4> jointData(invBindPos.size());

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
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i];
		vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);

			SubmitDrawCallLayer(*loadedMesh, j, frameCmds);
		}
	}
}