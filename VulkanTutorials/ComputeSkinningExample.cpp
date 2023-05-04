/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "ComputeSkinningExample.h"
#include "../NCLCoreClasses/MeshAnimation.h"

using namespace NCL;
using namespace Rendering;

ComputeSkinningExample::ComputeSkinningExample(Window& window) : VulkanTutorialRenderer(window)
{
	autoBeginDynamicRendering = false;
}

ComputeSkinningExample::~ComputeSkinningExample() {
	vkDeviceWaitIdle(GetDevice());
}

void ComputeSkinningExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	loader.Load("CesiumMan/CesiumMan.gltf",
		[](void) ->  MeshGeometry* {return new VulkanMesh(); },
		[&](std::string& input) ->  VulkanTexture* {return VulkanTexture::TextureFromFile(this, input).release(); }
	);

	camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	VulkanMesh* mesh = (VulkanMesh*)loader.outMeshes[0];
	mesh->UploadToGPU(this);

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

	drawShader = VulkanShaderBuilder("Texturing Shader")
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build(GetDevice());

	drawPipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(drawShader)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithDescriptorSetLayout(0,*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)  //Textures are set 1
	.Build(GetDevice());

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = VulkanBufferBuilder(sizeof(Matrix4) * matCount, "Joint Matrices")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(GetDevice(), GetMemoryAllocator());

	computeLayout = VulkanDescriptorSetLayoutBuilder("Compute Data")
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //0: In positions
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //1: In weights
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //2: In indices
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //3: Out positions
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //4: joint Matrices
		.Build(GetDevice()); //Get our camera matrices...

	computeDescriptor = BuildUniqueDescriptorSet(*computeLayout);

	skinShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "ComputeSkinning.comp.spv"));

	computePipeline = VulkanComputePipelineBuilder("Async Skinning")
		.WithShader(skinShader)
		.WithDescriptorSetLayout(0, *computeLayout)
		.Build(GetDevice());

	verticesBuffer = VulkanBufferBuilder(sizeof(Vector3) * mesh->GetVertexCount(), "Transformed Positions")
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithBufferUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.Build(GetDevice(), GetMemoryAllocator());

	const vk::PipelineVertexInputStateCreateInfo& meshInfo = mesh->GetVertexInputState();

	const VulkanBuffer* vertexBuffer = nullptr;
	const VulkanBuffer* weightBuffer = nullptr;
	const VulkanBuffer* indexBuffer = nullptr;

	uint32_t vertexOffset = 0;
	uint32_t weightOffset = 0;
	uint32_t indexOffset = 0;

	uint32_t vertexRange = 0;
	uint32_t weightRange = 0;
	uint32_t indexRange = 0;

	if (!mesh->GetAttributeInformation(VertexAttribute::Positions, &vertexBuffer, vertexOffset, vertexRange) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointWeights, &weightBuffer, weightOffset, weightRange) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointIndices, &indexBuffer, indexOffset, indexRange)
		) {
		//??
	}

	UpdateBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);

	UpdateBufferDescriptor(*computeDescriptor, 0, vk::DescriptorType::eStorageBuffer, *vertexBuffer, vertexOffset, vertexRange);
	UpdateBufferDescriptor(*computeDescriptor, 1, vk::DescriptorType::eStorageBuffer, *weightBuffer, weightOffset, weightRange);
	UpdateBufferDescriptor(*computeDescriptor, 2, vk::DescriptorType::eStorageBuffer, *indexBuffer, indexOffset, indexRange);
	UpdateBufferDescriptor(*computeDescriptor, 3, vk::DescriptorType::eStorageBuffer, verticesBuffer);
	UpdateBufferDescriptor(*computeDescriptor, 4, vk::DescriptorType::eStorageBuffer, jointsBuffer);

	computeSemaphore = GetDevice().createSemaphoreUnique({});
}

void ComputeSkinningExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceExtensions.emplace_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

	static vk::PhysicalDeviceScalarBlockLayoutFeatures	blockLayourFeatures(true);
	deviceFeatures.pNext = (void*)&blockLayourFeatures;
}

void ComputeSkinningExample::Update(float dt) {
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

void ComputeSkinningExample::RenderFrame() {
	VulkanMesh* mesh = (VulkanMesh*)loader.outMeshes[0];

	//We need to set up the compute skinning

	vk::CommandBuffer computeCmds = BeginCmdBuffer(CommandBufferType::AsyncCompute);

	computeCmds.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	computeCmds.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*computeDescriptor, 0, nullptr);

	computeCmds.dispatch(mesh->GetVertexCount(), 1, 1);

	SubmitCmdBuffer(computeCmds, CommandBufferType::AsyncCompute, {}, {}, * computeSemaphore);

	//Now to render the mesh!
	vk::CommandBuffer renderCmds = BeginCmdBuffer(CommandBufferType::Graphics);

	renderCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, drawPipeline);
	renderCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *drawPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	Matrix4 modelMatrix;
	renderCmds.pushConstants(*drawPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&modelMatrix);

	vector<vk::UniqueDescriptorSet>& set = layerDescriptors[0];

	mesh->BindToCommandBuffer(renderCmds);

	vk::DeviceSize offset = 0;
	renderCmds.bindVertexBuffers(0, 1, &verticesBuffer.buffer, &offset);

	BeginDefaultRendering(renderCmds);

	for (unsigned int j = 0; j < mesh->GetSubMeshCount(); ++j) {
		renderCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *drawPipeline.layout, 1, 1, &*set[j], 0, nullptr);

		const SubMesh* sm = mesh->GetSubMesh(j);

		if (mesh->GetIndexCount() > 0) {
			renderCmds.drawIndexed(sm->count, 1, sm->start, sm->base, 0);
		}
		else {
			renderCmds.draw(sm->count, 1, sm->start, 0);
		}
	}
	EndRendering(renderCmds);

	SubmitCmdBuffer(renderCmds, CommandBufferType::Graphics, {}, *computeSemaphore, {});
}