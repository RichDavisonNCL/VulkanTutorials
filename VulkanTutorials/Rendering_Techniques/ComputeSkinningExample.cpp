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

TUTORIAL_ENTRY(ComputeSkinningExample)

ComputeSkinningExample::ComputeSkinningExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)
{
	m_vkInit.autoBeginDynamicRendering = false;
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);

	m_camera.SetPitch(0.0f)
		.SetYaw(200)
		.SetPosition({ 0, 0, -2 })
		.SetFarPlane(5000.0f);

	VulkanMesh* mesh = (VulkanMesh*)scene.meshes[0].get();
	UploadMeshWait(*mesh, vk::BufferUsageFlagBits::eStorageBuffer);

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

	drawPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(mesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("SimpleVertexTransform.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("SingleTexture.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
		.WithDescriptorSetLayout(0,*m_cameraLayout)	//Camera is set 0
		.WithDescriptorSetLayout(1,*textureLayout)  //Textures are set 1
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

	computeLayout = DescriptorSetLayoutBuilder(context.device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eCompute) //0: In positions
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eCompute) //1: In weights
		.WithStorageBuffers(2, 1, vk::ShaderStageFlagBits::eCompute) //2: In indices
		.WithStorageBuffers(3, 1, vk::ShaderStageFlagBits::eCompute) //3: Out positions
		.WithStorageBuffers(4, 1, vk::ShaderStageFlagBits::eCompute) //4: joint Matrices
		.Build("Compute Data"); //Get our m_camera matrices...

	computeDescriptor = CreateDescriptorSet(context.device, context.descriptorPool,  *computeLayout);

	computePipeline = ComputePipelineBuilder(context.device)
		.WithShaderBinary("ComputeSkinning.comp.spv")
		.WithDescriptorSetLayout(0, *computeLayout)
		.Build("Async Skinning");

	outputVertices = m_memoryManager->CreateBuffer(
		{
				.size	= sizeof(Vector3) * mesh->GetVertexCount(),
				.usage	=	vk::BufferUsageFlagBits::eStorageBuffer |
							vk::BufferUsageFlagBits::eVertexBuffer
		},
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		"Transformed Positions"
	);

	const vk::PipelineVertexInputStateCreateInfo& meshInfo = mesh->GetVertexInputState();

	vk::Buffer vertexBuffer;
	vk::Buffer weightBuffer;
	vk::Buffer indexBuffer;

	vk::Format vertexFormat;
	vk::Format weightFormat;
	vk::Format indexFormat;

	uint32_t vertexOffset	= 0;
	uint32_t weightOffset	= 0;
	uint32_t indexOffset	= 0;

	uint32_t vertexRange	= 0;
	uint32_t weightRange	= 0;
	uint32_t indexRange		= 0;

	if (!mesh->GetAttributeInformation(VertexAttribute::Positions, vertexBuffer, vertexOffset, vertexRange, vertexFormat) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointWeights, weightBuffer, weightOffset, weightRange, weightFormat) ||
		!mesh->GetAttributeInformation(VertexAttribute::JointIndices, indexBuffer, indexOffset, indexRange, indexFormat)
		) {
		//??
	}

	WriteBufferDescriptor(context.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);

	WriteBufferDescriptor(context.device, *computeDescriptor, 0, vk::DescriptorType::eStorageBuffer, vertexBuffer, vertexOffset, vertexRange);
	WriteBufferDescriptor(context.device, *computeDescriptor, 1, vk::DescriptorType::eStorageBuffer, weightBuffer, weightOffset, weightRange);
	WriteBufferDescriptor(context.device, *computeDescriptor, 2, vk::DescriptorType::eStorageBuffer, indexBuffer, indexOffset, indexRange);
	WriteBufferDescriptor(context.device, *computeDescriptor, 3, vk::DescriptorType::eStorageBuffer, outputVertices);
	WriteBufferDescriptor(context.device, *computeDescriptor, 4, vk::DescriptorType::eStorageBuffer, jointsBuffer);

	computeSemaphore = context.device.createSemaphoreUnique({});
	asyncCmds  = CmdBufferCreate(context.device, context.commandPools[CommandType::AsyncCompute], "Async cmds");
	renderCmds = CmdBufferCreate(context.device, context.commandPools[CommandType::Graphics], "Gfx cmds");
}

void ComputeSkinningExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::Queue	asyncQueue	= context.queues[CommandType::AsyncCompute];
	vk::Queue	gfxQueue	= context.queues[CommandType::Graphics];

	VulkanMesh* mesh = (VulkanMesh*)scene.meshes[0].get();

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
	renderCmds->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *drawPipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);

	Matrix4 modelMatrix;
	renderCmds->pushConstants(*drawPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&modelMatrix);

	std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[0];

	mesh->BindToCommandBuffer(*renderCmds);

	vk::DeviceSize offset = 0;
	renderCmds->bindVertexBuffers(0, 1, &outputVertices.buffer, &offset);

	m_renderer->BeginRenderToScreen(*renderCmds);

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