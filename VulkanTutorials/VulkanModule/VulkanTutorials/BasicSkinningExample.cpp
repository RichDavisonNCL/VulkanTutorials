/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "BasicSkinningExample.h"

using namespace NCL;
using namespace Rendering;

BasicSkinningExample::BasicSkinningExample(Window& window) : VulkanTutorialRenderer(window) {
	cameraUniform.camera.SetPitch(0.0f)
		.SetYaw(160)
		.SetPosition(Vector3(3, 0, -12))
		.SetFarPlane(5000.0f);

	//loader = GLTFLoader("RiggedSimple/RiggedSimple.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); });
	loader = GLTFLoader("CesiumMan/CesiumMan.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); });
	//loader = GLTFLoader("SimpleSkin/SimpleSkin.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); });
	for (const auto& m : loader.outMeshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m;
		loadedMesh->UploadToGPU(this);
	}

	textureLayout = VulkanDescriptorSetLayoutBuilder()
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.WithDebugName("Object Textures")
	.BuildUnique(device);

	for (const auto& m : loader.outMats) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(BuildUniqueDescriptorSet(textureLayout.get()));
			UpdateImageDescriptor(matSet.back().get(), 0, ((VulkanTexture*)l.diffuse)->GetDefaultView(), *defaultSampler);
		}
	}
	shader = VulkanShaderBuilder()
		.WithVertexBinary("BasicSkinning.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
		.WithDebugName("Texturing Shader")
		.Build(device);

	jointsLayout = VulkanDescriptorSetLayoutBuilder()
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.WithDebugName("Joint Data")
	.Build(device); //Get our camera matrices...

	jointsDescriptor = BuildDescriptorSet(jointsLayout);

	VulkanMesh* m = (VulkanMesh*)loader.outMeshes[0];
	pipeline = VulkanPipelineBuilder()
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexSpecification(m->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, false)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithPass(defaultRenderPass)
		.WithDescriptorSetLayout(cameraLayout.get())	//Camera is set 0
		.WithDescriptorSetLayout(textureLayout.get())	//Textures are set 1
		.WithDescriptorSetLayout(jointsLayout)	//Joints are set 2
		.WithDebugName("Main Scene Pipeline")
		.Build(device, pipelineCache);

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = CreateBuffer(sizeof(Matrix4) * matCount,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible);

	vector<Matrix4> bindPose	= loader.outMeshes[0]->GetBindPose();
	vector<Matrix4> invBindPos	= loader.outMeshes[0]->GetInverseBindPose();

	vector<Matrix4> testData;

	for (int i = 0; i < bindPose.size(); ++i) {
		testData.push_back(bindPose[i] * invBindPos[i]);
	}

	UploadBufferData(jointsBuffer, testData.data(), sizeof(Matrix4) * matCount);

	vk::WriteDescriptorSet descriptorWrites = vk::WriteDescriptorSet()
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setDstSet(jointsDescriptor)
		.setDstBinding(0)//0?
		.setDescriptorCount(1)
		.setPBufferInfo(&jointsBuffer.descriptorInfo);

	device.updateDescriptorSets(1, &descriptorWrites, 0, nullptr);

	frameTime		= 1 / 30.0f;
	currentFrame	= 0;
}

BasicSkinningExample::~BasicSkinningExample() {
	delete shader;
}

void BasicSkinningExample::RenderFrame() {
	BeginDefaultRenderPass(defaultCmdBuffer);
	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());
	UpdateUniformBufferDescriptor(cameraDescriptor.get(), cameraUniform.cameraData, 0);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 0, 1, &cameraDescriptor.get(), 0, nullptr);
	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 2, 1, &jointsDescriptor, 0, nullptr);

	for (int i = 0; i < loader.outMeshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i];
		vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 1, 1, &set[j].get(), 0, nullptr);

			SubmitDrawCallLayer(loadedMesh, j, defaultCmdBuffer);
		}
	}
	defaultCmdBuffer.endRenderPass();
}


void BasicSkinningExample::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	frameTime -= dt;

	if (frameTime <= 0.0f) {
		currentFrame = (currentFrame + 1) % loader.outAnims[0].GetFrameCount();
		frameTime += loader.outAnims[0].GetFrameRate();
	}

	vector<Matrix4> bindPose = loader.outMeshes[0]->GetBindPose();
	vector<Matrix4> invBindPos = loader.outMeshes[0]->GetInverseBindPose();

	const Matrix4* frameMats = loader.outAnims[0].GetJointData(currentFrame);

	vector<Matrix4> testData;

	for (int i = 0; i < bindPose.size(); ++i) {
		testData.push_back(frameMats[i] * invBindPos[i]);
	}
	int dataSize = sizeof(Matrix4) * loader.outMeshes[0]->GetJointCount();
	UploadBufferData(jointsBuffer, testData.data(), dataSize);
}