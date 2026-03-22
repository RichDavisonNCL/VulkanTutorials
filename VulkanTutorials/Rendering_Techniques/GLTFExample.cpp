/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "GLTFExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(GLTFExample)

GLTFExample::GLTFExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit)	{
	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	m_camera.SetPitch(-20.0f)
		.SetYaw(90.0f)
		.SetPosition({ 850, 840, -30 })
		.SetFarPlane(5000.0f);

	GLTFLoader::Load("Sponza/Sponza.gltf",scene);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		UploadMeshWait(*loadedMesh);
	}

	VulkanMesh* m = (VulkanMesh*)scene.meshes[0].get();
	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("SimpleVertexTransform.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("SingleTexture.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)
	.Build("Main Scene Pipeline");

	for (const auto& m : scene.meshMaterials) {	//Build descriptors for each mesh and its sublayers
		layerDescriptors.push_back({});
		std::vector<vk::UniqueDescriptorSet>& matSet = layerDescriptors.back();
		for (const auto& l : m.layers) {
			GLTFMaterial& layer = scene.materials[l];
			matSet.push_back(CreateDescriptorSet(context.device, context.descriptorPool, pipeline.GetSetLayout(1)));
			VulkanTexture* layerTex = ((VulkanTexture*)layer.albedo.get());
			WriteCombinedImageDescriptor(context.device, *matSet.back(), 0, layerTex->GetDefaultView(), *m_defaultSampler);
		}
	}
	WriteBufferDescriptor(context.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
}

void GLTFExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	Matrix4 identity;
	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&identity);

	context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*m_cameraDescriptor, 0, nullptr);

	for(size_t i = 0; i < scene.meshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)scene.meshes[i].get();
		std::vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			context.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);
		
			loadedMesh->DrawLayer(j, context.cmdBuffer);
		}
	}
}