/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "GLTFExample.h"

using namespace NCL;
using namespace Rendering;

GLTFExample::GLTFExample(Window& window) : VulkanTutorialRenderer(window)
,loader("Sponza/Sponza.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); })
{
	cameraUniform.camera.SetPitch(-20.0f)
	.SetYaw(90.0f)
	.SetPosition(Vector3(850, 840, -30))
	.SetFarPlane(5000.0f);

	//loader = GLTFLoader("Sponza/Sponza.gltf", [](void) ->  MeshGeometry* {return new VulkanMesh(); });

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
		.WithVertexBinary("SimpleVertexTransform.vert.spv")
		.WithFragmentBinary("SingleTexture.frag.spv")
		.WithDebugName("Texturing Shader")
	.BuildUnique(device);

	VulkanMesh* m = (VulkanMesh*)loader.outMeshes[0];
	pipeline = VulkanPipelineBuilder()
		.WithPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4))
		.WithVertexSpecification(m->GetVertexSpecification())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderState(shader.get())
		.WithBlendState(vk::BlendFactor::eOne, vk::BlendFactor::eOne, false)
		.WithDepthState(vk::CompareOp::eLessOrEqual, true, true, false)
		.WithColourFormats({ surfaceFormat })
		.WithDepthStencilFormat(depthBuffer->GetFormat())
		.WithDescriptorSetLayout(cameraLayout.get())	//Camera is set 0
		.WithDescriptorSetLayout(textureLayout.get())	//Textures are set 1
		.WithDebugName("Main Scene Pipeline")
	.Build(device, pipelineCache);
}

GLTFExample::~GLTFExample() {
}

void GLTFExample::RenderFrame() {
	TransitionSwapchainForRendering(defaultCmdBuffer);
	BeginDefaultRendering(defaultCmdBuffer);

	defaultCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());
	Matrix4 identity;
	defaultCmdBuffer.pushConstants(pipeline.layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&identity);

	defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 0, 1, &cameraDescriptor.get(), 0, nullptr);
	UpdateUniformBufferDescriptor(cameraDescriptor.get(), cameraUniform.cameraData, 0);
	for(int i = 0; i < loader.outMeshes.size(); ++i) {
		VulkanMesh* loadedMesh = (VulkanMesh*)loader.outMeshes[i];
		vector<vk::UniqueDescriptorSet>& set = layerDescriptors[i];

		for (unsigned int j = 0; j < loadedMesh->GetSubMeshCount(); ++j) {
			defaultCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout.get(), 1, 1, &set[j].get(), 0, nullptr);
		
			SubmitDrawCallLayer(loadedMesh, j, defaultCmdBuffer);
		}
	}
	EndRendering(defaultCmdBuffer);
}