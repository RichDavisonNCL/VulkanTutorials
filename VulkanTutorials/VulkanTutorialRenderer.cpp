/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanTutorialRenderer.h"

using namespace NCL;
using namespace Rendering;

VulkanTutorialRenderer::VulkanTutorialRenderer(Window& window) : VulkanRenderer(window) {
	majorVersion = 1;
	minorVersion = 3;
}

VulkanTutorialRenderer::~VulkanTutorialRenderer() {
}

void VulkanTutorialRenderer::SetupTutorial() {
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

	UpdateBufferDescriptor(*cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);

	runTime = 0.0f;

	nullLayout = VulkanDescriptorSetLayoutBuilder().Build(GetDevice());

	Vulkan::SetNullDescriptor(GetDevice(), *nullLayout);
}

void VulkanTutorialRenderer::BuildCamera() {
	camera			= Camera::BuildPerspectiveCamera(Vector3(0, 0, 1), 0, 0, 45.0f, 0.1f, 1000.0f);
	cameraBuffer = VulkanBufferBuilder(sizeof(Matrix4) * 2)
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(GetDevice(), GetMemoryAllocator());
}

void VulkanTutorialRenderer::UpdateCamera(float dt) {
	camera.UpdateCamera(dt);
}

void VulkanTutorialRenderer::UploadCameraUniform() {
	Matrix4* cameraMatrices = (Matrix4*)cameraBuffer.Data();
	cameraMatrices[0] = camera.BuildViewMatrix();
	cameraMatrices[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());
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
	newMesh->UploadToGPU(this);
	return UniqueVulkanMesh(newMesh);
}

void VulkanTutorialRenderer::RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet) {
	toBuffer.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&o.transform);
	toBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *toPipeline.layout, descriptorSet, 1, &*o.objectDescriptorSet, 0, nullptr);
	
	if (o.mesh->GetSubMeshCount() == 0) {
		SubmitDrawCall(toBuffer, *o.mesh);
	}
	else {
		for (unsigned int i = 0; i < o.mesh->GetSubMeshCount(); ++i) {
			SubmitDrawCallLayer(*o.mesh, i, toBuffer);
		}
	}
}