/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanTutorialRenderer.h"

using namespace NCL;
using namespace Rendering;

VulkanTutorialRenderer::VulkanTutorialRenderer(Window& window, VulkanInitInfo info) : VulkanRenderer(window, info) {
	BuildCamera();
	defaultSampler = GetDevice().createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(16)
		.setMinFilter(vk::Filter::eLinear)
		.setMagFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMaxLod(80.0f)
	);

	cameraLayout = VulkanDescriptorSetLayoutBuilder("CameraMatrices")
		.WithUniformBuffers(1, vk::ShaderStageFlagBits::eVertex)
		.Build(GetDevice()); //Get our camera matrices...
	cameraDescriptor = BuildUniqueDescriptorSet(*cameraLayout);

	UpdateBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);

	runTime = 0.0f;

	nullLayout = VulkanDescriptorSetLayoutBuilder().Build(device);
}

VulkanTutorialRenderer::~VulkanTutorialRenderer() {
	DestroyBuffer(cameraUniform.cameraData);
}

void VulkanTutorialRenderer::BuildCamera() {
	cameraUniform.camera		= Camera::BuildPerspectiveCamera(Vector3(0, 0, 1), 0, 0, 45.0f, 0.1f, 1000.0f);
	cameraUniform.cameraData	= CreateBuffer(sizeof(Matrix4) * 2, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
	cameraUniform.cameraMemory	= (Matrix4*)GetDevice().mapMemory(*cameraUniform.cameraData.deviceMem, 0, cameraUniform.cameraData.allocInfo.allocationSize);
}

void VulkanTutorialRenderer::UpdateCamera(float dt) {
	cameraUniform.camera.UpdateCamera(dt);
}

void VulkanTutorialRenderer::UploadCameraUniform() {
	cameraUniform.cameraMemory[0] = cameraUniform.camera.BuildViewMatrix();
	cameraUniform.cameraMemory[1] = cameraUniform.camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());
}

UniqueVulkanMesh VulkanTutorialRenderer::GenerateTriangle() {
	VulkanMesh* triMesh = new VulkanMesh();
	triMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(0,1,0) });
	triMesh->SetVertexColours({ Vector4(1,0,0,1), Vector4(0,1,0,1), Vector4(0,0,1,1) });
	triMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(0.5, 1) });
	triMesh->SetVertexIndices({ 0,1,2 });

	triMesh->SetDebugName("Triangle");
	triMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);
	triMesh->UploadToGPU(this);

	return UniqueVulkanMesh(triMesh);
}

UniqueVulkanMesh VulkanTutorialRenderer::GenerateQuad() {
	VulkanMesh* quadMesh = new VulkanMesh();
	quadMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	quadMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	quadMesh->SetVertexIndices({ 0,1,3,2 });
	quadMesh->SetDebugName("Fullscreen Quad");
	quadMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);
	quadMesh->UploadToGPU(this);

	return UniqueVulkanMesh(quadMesh);
}

UniqueVulkanMesh VulkanTutorialRenderer::GenerateGrid() {
	VulkanMesh* gridMesh = new VulkanMesh();
	gridMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	gridMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	gridMesh->SetVertexIndices({ 0,1,3,2 });
	gridMesh->SetDebugName("Test Grid");
	gridMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);
	gridMesh->UploadToGPU(this);

	return UniqueVulkanMesh(gridMesh);
}

UniqueVulkanMesh VulkanTutorialRenderer::LoadMesh(const string& filename) {
	VulkanMesh* newMesh = new VulkanMesh(filename);
	newMesh->SetPrimitiveType(NCL::GeometryPrimitive::Triangles);
	newMesh->SetDebugName(filename);
	newMesh->UploadToGPU(this);
	return UniqueVulkanMesh(newMesh);
}

VulkanFrameBuffer VulkanTutorialRenderer::BuildFrameBuffer(vk::RenderPass& pass, int width, int height, vector<std::shared_ptr<VulkanTexture>> colourAttachments, std::shared_ptr<VulkanTexture> depthAttacment) {
	VulkanFrameBuffer buffer;
	vector<vk::ImageView> allViews;

	for (uint32_t i = 0; i < colourAttachments.size(); ++i) {
		buffer.colourAttachments.emplace_back(colourAttachments[i]);
		allViews.emplace_back(buffer.colourAttachments.back()->GetDefaultView());
	}
	if (depthAttacment) {
		buffer.depthAttachment = depthAttacment;
		allViews.emplace_back(buffer.depthAttachment->GetDefaultView());
	}
	if (!pass) { //uh oh, we weren't given a valid pass, never mind, we can make one!
		pass = BuildPassForFrameBuffer(buffer);
	}
	vk::FramebufferCreateInfo createInfo = vk::FramebufferCreateInfo()
		.setWidth(width)
		.setHeight(height)
		.setLayers(1)
		.setAttachmentCount((uint32_t)allViews.size())
		.setPAttachments(allViews.data())
		.setRenderPass(pass);

	buffer.frameBuffer = GetDevice().createFramebufferUnique(createInfo);
	buffer.width = width;
	buffer.height = height;
	return buffer;
}

VulkanFrameBuffer VulkanTutorialRenderer::BuildFrameBuffer(vk::RenderPass& pass, int width, int height, uint32_t colourCount, bool hasDepth, bool floatTex, bool stencilDepth) {
	VulkanFrameBuffer buffer;

	vector<vk::ImageView> allViews;

	for (uint32_t i = 0; i < colourCount; ++i) {
		buffer.colourAttachments.emplace_back(VulkanTexture::CreateColourTexture(width, height));
		allViews.emplace_back(buffer.colourAttachments.back()->GetDefaultView());
	}

	if (hasDepth) {
		buffer.depthAttachment = VulkanTexture::CreateDepthTexture(width, height, "DefaultDepth", stencilDepth);
		allViews.emplace_back(buffer.depthAttachment->GetDefaultView());
	}

	if (!pass) { //uh oh, we weren't given a valid pass, never mind, we can make one!
		pass = BuildPassForFrameBuffer(buffer);
		//buffer.createdPass = pass;
	}

	vk::FramebufferCreateInfo createInfo = vk::FramebufferCreateInfo()
		.setWidth(width)
		.setHeight(height)
		.setLayers(1)
		.setAttachmentCount((uint32_t)allViews.size())
		.setPAttachments(allViews.data())
		.setRenderPass(pass);

	buffer.frameBuffer	= GetDevice().createFramebufferUnique(createInfo);
	buffer.width		= width;
	buffer.height		= height;
	return buffer;
}

vk::RenderPass VulkanTutorialRenderer::BuildPassForFrameBuffer(VulkanFrameBuffer& buffer, bool clearColours, bool clearDepth) {
	vector<vk::AttachmentDescription>	allDescriptions;
	vector<vk::AttachmentReference>		allReferences;
	vk::AttachmentReference depthReference;

	int currentAttachment = 0;
	for (auto& i : buffer.colourAttachments) {
		allDescriptions.emplace_back(
			vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFormat(i->GetFormat())
			.setLoadOp(vk::AttachmentLoadOp::eClear)
		);
		allReferences.emplace_back(vk::AttachmentReference(currentAttachment, vk::ImageLayout::eColorAttachmentOptimal));
		currentAttachment++;
	}

	vk::SubpassDescription subPass = vk::SubpassDescription()
		.setColorAttachmentCount((uint32_t)allReferences.size())
		.setPColorAttachments(allReferences.data())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	if (buffer.depthAttachment) {	
		vk::AttachmentReference depthReference = vk::AttachmentReference(currentAttachment, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		allDescriptions.emplace_back(
			vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFormat(buffer.depthAttachment->GetFormat())
			.setLoadOp(vk::AttachmentLoadOp::eClear)
		);
		subPass.setPDepthStencilAttachment(&depthReference);
	}

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount((uint32_t)allDescriptions.size())
		.setPAttachments(allDescriptions.data())
		.setSubpassCount(1)
		.setPSubpasses(&subPass);

	return GetDevice().createRenderPass(renderPassInfo);
}

void VulkanTutorialRenderer::RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet) {
	toBuffer.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&o.transform);
	toBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *toPipeline.layout, descriptorSet, 1, &*o.objectDescriptorSet, 0, nullptr);
	
	if (o.mesh->GetSubMeshCount() == 0) {
		SubmitDrawCall(*o.mesh, toBuffer);
	}
	else {
		for (unsigned int i = 0; i < o.mesh->GetSubMeshCount(); ++i) {
			SubmitDrawCallLayer(*o.mesh, i, toBuffer);
		}
	}
}

void VulkanTutorialRenderer::TransitionColourToSampler(VulkanTexture* t, vk::CommandBuffer  buffer) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);
}

void VulkanTutorialRenderer::TransitionDepthToSampler(VulkanTexture* t, vk::CommandBuffer  buffer, bool doStencil) {
	vk::ImageAspectFlags flags = doStencil ? vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eDepth;

	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal, flags,
		vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::PipelineStageFlagBits::eFragmentShader);
}

void VulkanTutorialRenderer::TransitionSamplerToColour(VulkanTexture* t, vk::CommandBuffer  buffer) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

void VulkanTutorialRenderer::TransitionSamplerToDepth(VulkanTexture* t, vk::CommandBuffer  buffer, bool doStencil) {
	vk::ImageAspectFlags flags = doStencil ? vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eDepth;

	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal, flags,
		vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eEarlyFragmentTests);
}