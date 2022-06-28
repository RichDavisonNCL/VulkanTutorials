/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BasicSkinningExample.h"
#include "../../Common/MeshAnimation.h"

using namespace NCL;
using namespace Rendering;

BasicSkinningExample::BasicSkinningExample(Window& window) : VulkanTutorialRenderer(window) 
,loader("CesiumMan/CesiumMan.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); })
{
	cameraUniform.camera.SetPitch(0.0f)
		.SetYaw(160)
		.SetPosition({ 3, 0, -12 })
		.SetFarPlane(5000.0f);

	for (const auto& m : loader.outMeshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m;
		loadedMesh->UploadToGPU(this);
	}

	textureLayout = VulkanDescriptorSetLayoutBuilder("Object Textures")
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.Build(device);

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
	.Build(device);

	jointsLayout = VulkanDescriptorSetLayoutBuilder("Joint Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(device); //Get our camera matrices...

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
		.WithDescriptorSetLayout(*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(*textureLayout)//Textures are set 1
		.WithDescriptorSetLayout(*jointsLayout)	//Joints are set 2
	.Build(device, pipelineCache);

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = CreateBuffer(sizeof(Matrix4) * matCount,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible);

	vector<Matrix4> bindPose	= loader.outMeshes[0]->GetBindPose();
	vector<Matrix4> invBindPos	= loader.outMeshes[0]->GetInverseBindPose();

	vector<Matrix4> testData;

	for (size_t i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	UploadBufferData(jointsBuffer, testData.data(), sizeof(Matrix4) * matCount);

	UpdateBufferDescriptor(*jointsDescriptor, jointsBuffer, 0, vk::DescriptorType::eStorageBuffer);
	UpdateBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
	frameTime		= 1 / 30.0f;
	currentFrame	= 0;
}

void BasicSkinningExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 2, 1, &*jointsDescriptor, 0, nullptr);

	for (size_t i = 0; i < loader.outMeshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i];
		vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);

			SubmitDrawCallLayer(*loadedMesh, j, defaultCmdBuffer);
		}
	}
	EndRendering(defaultCmdBuffer);
}

void BasicSkinningExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	frameTime -= dt;

	if (frameTime <= 0.0f) {
		currentFrame = (currentFrame + 1) % loader.outAnims[0]->GetFrameCount();
		frameTime += loader.outAnims[0]->GetFrameRate();
	}

	vector<Matrix4> bindPose = loader.outMeshes[0]->GetBindPose();
	vector<Matrix4> invBindPos = loader.outMeshes[0]->GetInverseBindPose();

	const Matrix4* frameMats = loader.outAnims[0]->GetJointData(currentFrame);

	vector<Matrix4> testData;

	for (int i = 0; i < bindPose.size(); ++i) {
		testData.push_back(frameMats[i] * invBindPos[i]);
	}
	int dataSize = sizeof(Matrix4) * loader.outMeshes[0]->GetJointCount();
	UploadBufferData(jointsBuffer, testData.data(), dataSize);
}