/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "GLTFExample.h"

using namespace NCL;
using namespace Rendering;

GLTFExample::GLTFExample(Window& window) : VulkanTutorialRenderer(window)
{
}

void GLTFExample::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();
	loader.Load("Sponza/Sponza.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); });

	cameraUniform.camera.SetPitch(-20.0f)
		.SetYaw(90.0f)
		.SetPosition({ 850, 840, -30 })
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
			UpdateImageDescriptor(*matSet.back(), 0, 0, ((VulkanTexture*)l.diffuse)->GetDefaultView(), *defaultSampler);
		}
	}

	shader = VulkanShaderBuilder("Texturing Shader")
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
		.Build(device);

	VulkanMesh* m = (VulkanMesh*)loader.outMeshes[0];
	pipeline = VulkanPipelineBuilder("Main Scene Pipeline")
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShader(shader)
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, false)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(0, *cameraLayout)		//Camera is set 0
		.WithDescriptorSetLayout(1, *textureLayout)	//Textures are set 1
		.Build(device);
}


void GLTFExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	Matrix4 identity;
	defaultCmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&identity);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, 1, &*cameraDescriptor, 0, nullptr);
	UpdateBufferDescriptor(*cameraDescriptor, cameraUniform.cameraData, 0, vk::DescriptorType::eUniformBuffer);
	for(size_t i = 0; i < loader.outMeshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i];
		vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 1, 1, &*set[j], 0, nullptr);
		
			SubmitDrawCallLayer(*loadedMesh, j, defaultCmdBuffer);
		}
	}
	EndRendering(defaultCmdBuffer);
}