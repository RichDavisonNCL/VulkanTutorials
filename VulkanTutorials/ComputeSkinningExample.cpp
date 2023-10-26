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
using namespace Vulkan;

ComputeSkinningExample::ComputeSkinningExample(Window& window) : VulkanTutorialRenderer(window)
{
	autoBeginDynamicRendering = false;
}

ComputeSkinningExample::~ComputeSkinningExample() {
	GetDevice().waitIdle();
}

void ComputeSkinningExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	loader.Load("CesiumMan/CesiumMan.gltf",
		[](void) ->  Mesh* {return new VulkanMesh(); },
		[&](std::string& input) ->  VulkanTexture* {return LoadTexture(input).release(); }
	);

	camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	VulkanMesh* mesh = (VulkanMesh*)loader.outMeshes[0].get();
	mesh->UploadToGPU(this);

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

	drawShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	drawPipeline = PipelineBuilder(GetDevice())
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(drawShader)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0,*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)  //Textures are set 1
	.Build("Main Scene Pipeline");

	int matCount = loader.outMeshes[0]->GetJointCount();

	jointsBuffer = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * matCount, "Joint Matrices");

	computeLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //0: In positions
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //1: In weights
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //2: In indices
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //3: Out positions
		.WithStorageBuffers(1, vk::ShaderStageFlagBits::eCompute) //4: joint Matrices
		.Build("Compute Data"); //Get our camera matrices...

	computeDescriptor = BuildUniqueDescriptorSet(*computeLayout);

	skinShader = UniqueVulkanCompute(new VulkanCompute(GetDevice(), "ComputeSkinning.comp.spv"));

	computePipeline = ComputePipelineBuilder(GetDevice())
		.WithShader(skinShader)
		.WithDescriptorSetLayout(0, *computeLayout)
		.Build("Async Skinning");

	outputVertices = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithBufferUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.Build(sizeof(Vector3) * mesh->GetVertexCount(), "Transformed Positions");

	const vk::PipelineVertexInputStateCreateInfo& meshInfo = mesh->GetVertexInputState();

	vk::Buffer vertexBuffer;
	vk::Buffer weightBuffer;
	vk::Buffer indexBuffer;

	vk::Format vertexFormat;
	vk::Format weightFormat;
	vk::Format indexFormat;

	uint32_t vertexOffset = 0;
	uint32_t weightOffset = 0;
	uint32_t indexOffset = 0;

	uint32_t vertexRange = 0;
	uint32_t weightRange = 0;
	uint32_t indexRange = 0;

	if (!mesh->GetAttributeInformation(VertexAttribute::Positions, vertexBuffer, vertexOffset, vertexRange, vertexFormat) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointWeights, weightBuffer, weightOffset, weightRange, weightFormat) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointIndices, indexBuffer, indexOffset, indexRange, indexFormat)
		) {
		//??
	}

	WriteBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);

	WriteBufferDescriptor(*computeDescriptor, 0, vk::DescriptorType::eStorageBuffer, vertexBuffer, vertexOffset, vertexRange);
	WriteBufferDescriptor(*computeDescriptor, 1, vk::DescriptorType::eStorageBuffer, weightBuffer, weightOffset, weightRange);
	WriteBufferDescriptor(*computeDescriptor, 2, vk::DescriptorType::eStorageBuffer, indexBuffer, indexOffset, indexRange);
	WriteBufferDescriptor(*computeDescriptor, 3, vk::DescriptorType::eStorageBuffer, outputVertices);
	WriteBufferDescriptor(*computeDescriptor, 4, vk::DescriptorType::eStorageBuffer, jointsBuffer);

	computeSemaphore = GetDevice().createSemaphoreUnique({});
	vk::CommandPool asyncPool	= GetCommandPool(CommandBuffer::AsyncCompute);
	vk::CommandPool gfxPool		= GetCommandPool(CommandBuffer::Graphics);
	asyncCmds  = CmdBufferCreate(GetDevice(), asyncPool, "Async cmds");
	renderCmds = CmdBufferCreate(GetDevice(), gfxPool, "Gfx cmds");
}

void ComputeSkinningExample::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceExtensions.emplace_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

	static vk::PhysicalDeviceScalarBlockLayoutFeatures	blockLayourFeatures(true);
	deviceFeatures.pNext = (void*)&blockLayourFeatures;
}

void ComputeSkinningExample::Update(float dt) {
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

void ComputeSkinningExample::RenderFrame() {
	VulkanMesh* mesh = (VulkanMesh*)loader.outMeshes[0].get();

	vk::Queue		asyncQueue	= GetQueue(CommandBuffer::AsyncCompute);
	vk::Queue		gfxQueue	= GetQueue(CommandBuffer::Graphics);

	//We need to set up the compute skinning
	CmdBufferResetBegin(asyncCmds);
	asyncCmds->bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	asyncCmds->bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipeline.layout, 0, 1, &*computeDescriptor, 0, nullptr);
	asyncCmds->dispatch(mesh->GetVertexCount(), 1, 1);
	CmdBufferEndSubmit(*asyncCmds, asyncQueue, {}, {}, *computeSemaphore);

	////Now to render the mesh!
	CmdBufferResetBegin(renderCmds);
	renderCmds->bindPipeline(vk::PipelineBindPoint::eGraphics, drawPipeline);
	renderCmds->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *drawPipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);

	Matrix4 modelMatrix;
	renderCmds->pushConstants(*drawPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&modelMatrix);

	std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[0];

	mesh->BindToCommandBuffer(*renderCmds);

	vk::DeviceSize offset = 0;
	renderCmds->bindVertexBuffers(0, 1, &outputVertices.buffer, &offset);

	BeginDefaultRendering(*renderCmds);

	for (unsigned int j = 0; j < mesh->GetSubMeshCount(); ++j) {
		renderCmds->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *drawPipeline.layout, 1, 1, &*set[j], 0, nullptr);

		const SubMesh* sm = mesh->GetSubMesh(j);

		if (mesh->GetIndexCount() > 0) {
			renderCmds->drawIndexed(sm->count, 1, sm->start, sm->base, 0);
		}
		else {
			renderCmds->draw(sm->count, 1, sm->start, 0);
		}
	}
	renderCmds->endRendering();
	CmdBufferEndSubmit(*renderCmds, gfxQueue, {}, *computeSemaphore, {});
}