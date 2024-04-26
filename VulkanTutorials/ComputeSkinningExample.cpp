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

ComputeSkinningExample::ComputeSkinningExample(Window& window) : VulkanTutorial(window)
{
	VulkanInitialisation vkInit = DefaultInitialisation();
	vkInit.autoBeginDynamicRendering = false;
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

	VulkanMesh* mesh = (VulkanMesh*)scene.meshes[0].get();
	mesh->UploadToGPU(renderer);

	textureLayout = DescriptorSetLayoutBuilder(device)
		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
		.Build("Object Textures");

	for (const auto& m : scene.materials) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.allLayers) {
			matSet.push_back(CreateDescriptorSet(device, pool, *textureLayout));
			WriteImageDescriptor(device , *matSet.back(), 0,((VulkanTexture*)l.albedo.get())->GetDefaultView(), *defaultSampler);
		}
	}

	drawShader = ShaderBuilder(device)
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
	.Build("Texturing Shader");

	drawPipeline = PipelineBuilder(device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(drawShader)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0,*cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)  //Textures are set 1
	.Build("Main Scene Pipeline");

	int matCount = scene.meshes[0]->GetJointCount();

	jointsBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithHostVisibility()
		.Build(sizeof(Matrix4) * matCount, "Joint Matrices");

	computeLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eCompute) //0: In positions
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eCompute) //1: In weights
		.WithStorageBuffers(2, 1, vk::ShaderStageFlagBits::eCompute) //2: In indices
		.WithStorageBuffers(3, 1, vk::ShaderStageFlagBits::eCompute) //3: Out positions
		.WithStorageBuffers(4, 1, vk::ShaderStageFlagBits::eCompute) //4: joint Matrices
		.Build("Compute Data"); //Get our camera matrices...

	computeDescriptor = CreateDescriptorSet(device , pool,  *computeLayout);

	skinShader = UniqueVulkanCompute(new VulkanCompute(device, "ComputeSkinning.comp.spv"));

	computePipeline = ComputePipelineBuilder(device)
		.WithShader(skinShader)
		.WithDescriptorSetLayout(0, *computeLayout)
		.Build("Async Skinning");

	outputVertices = BufferBuilder(device, renderer->GetMemoryAllocator())
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

	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);

	WriteBufferDescriptor(device, *computeDescriptor, 0, vk::DescriptorType::eStorageBuffer, vertexBuffer, vertexOffset, vertexRange);
	WriteBufferDescriptor(device, *computeDescriptor, 1, vk::DescriptorType::eStorageBuffer, weightBuffer, weightOffset, weightRange);
	WriteBufferDescriptor(device, *computeDescriptor, 2, vk::DescriptorType::eStorageBuffer, indexBuffer, indexOffset, indexRange);
	WriteBufferDescriptor(device, *computeDescriptor, 3, vk::DescriptorType::eStorageBuffer, outputVertices);
	WriteBufferDescriptor(device, *computeDescriptor, 4, vk::DescriptorType::eStorageBuffer, jointsBuffer);

	computeSemaphore = device.createSemaphoreUnique({});
	vk::CommandPool asyncPool	= renderer->GetCommandPool(CommandBuffer::AsyncCompute);
	vk::CommandPool gfxPool		= renderer->GetCommandPool(CommandBuffer::Graphics);
	asyncCmds  = CmdBufferCreate(device, asyncPool, "Async cmds");
	renderCmds = CmdBufferCreate(device, gfxPool, "Gfx cmds");

}

ComputeSkinningExample::~ComputeSkinningExample() {
	renderer->GetDevice().waitIdle();
}

void ComputeSkinningExample::RenderFrame(float dt) {
	VulkanMesh* mesh = (VulkanMesh*)scene.meshes[0].get();

	vk::Queue		asyncQueue	= renderer->GetQueue(CommandBuffer::AsyncCompute);
	vk::Queue		gfxQueue	= renderer->GetQueue(CommandBuffer::Graphics);

	frameTime -= dt;

	//Mesh*  mesh = scene.meshes[0].get();
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

	renderer->BeginDefaultRendering(*renderCmds);

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