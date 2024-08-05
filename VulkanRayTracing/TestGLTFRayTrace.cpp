/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "TestGLTFRayTrace.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"
#include <algorithm>
#include <iterator>

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TestGLTFRayTrace::TestGLTFRayTrace(Window& window) : VulkanTutorial(window) {
	VulkanInitialisation vkInit = DefaultInitialisation();
	vkInit.majorVersion = 1;
	vkInit.minorVersion = 3;
	vkInit.autoBeginDynamicRendering = false;
	vkInit.skipDynamicState = true;

	vkInit.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	//https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_robustness2.html
	vkInit.deviceExtensions.emplace_back(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
	//https://www.khronos.org/blog/introducing-vulkan-ray-tracing-position-fetch-extension
	vkInit.deviceExtensions.emplace_back(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;



	static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
		.accelerationStructure = VK_TRUE
	};

	static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures = {
		.rayTracingPipeline = VK_TRUE
	};

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = VK_TRUE
	};

	static VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rtPositionFetchFeature = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR,
		.rayTracingPositionFetch = VK_TRUE
	};

	vkInit.features.push_back((void*)&accelFeatures);
	vkInit.features.push_back((void*)&rayFeatures);
	vkInit.features.push_back((void*)&deviceAddressfeature);
	vkInit.features.push_back((void*)&rtPositionFetchFeature);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& state = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	GLTFLoader::Load("Sponza/Sponza.gltf", scene);
	//GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);
	//GLTFLoader::Load("DamagedHelmet/DamagedHelmet.gltf", scene);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		//We are stroing mesh vertex information in VK_STORAGE_BUFFER bit ON.
		//We are enabling flag required for raytracing
		loadedMesh->UploadToGPU(renderer,	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
										vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
	}

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &rayPipelineProperties;
	renderer->GetPhysicalDevice().getProperties2(&props);

	raygenShader	= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/raygen.rgen.spv", device));
	hitShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/closesthit.rchit.spv", device));
	hitShader2		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/closesthit2.rchit.spv", device));
	missShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/miss.rmiss.spv", device));

	defaultTexture = LoadTexture("Doge.png");

	rayTraceLayout = DescriptorSetLayoutBuilder(device)
		.WithAccelStructures(0, 1)
		.Build("Ray Trace TLAS Layout");

	imageLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageImages(0, 1)
		.Build("Ray Trace Image Layout");

	inverseCamLayout = DescriptorSetLayoutBuilder(device)
		.WithUniformBuffers(0, 1)
		.Build("Camera Inverse Matrix Layout");

	defaultSamplerLayout = DescriptorSetLayoutBuilder(device)
		.WithImageSamplers(0, 1)
		.Build("Default Sampler");

	dSamplerLayout = DescriptorSetLayoutBuilder(device)
		.WithSamplers(0, 1)
		.Build("D Sampler!");

	BuildTlas(device);

	rayTraceDescriptor		= CreateDescriptorSet(device, pool, *rayTraceLayout);
	imageDescriptor			= CreateDescriptorSet(device, pool, *imageLayout);
	inverseCamDescriptor	= CreateDescriptorSet(device, pool, *inverseCamLayout);

	inverseMatrices = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * 2, "InverseMatrices");

	defaultSamplerDescriptor = CreateDescriptorSet(device, pool, *defaultSamplerLayout);
	dSamplerDescriptor = CreateDescriptorSet(device, pool, *dSamplerLayout);

	rtSceneBufferLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //Position
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //Index
		.WithStorageBuffers(2, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //Tex Corrd
		.WithStorageBuffers(3, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //Normal Corrd
		.WithSampledImages(4, scene.textures.size(), vk::ShaderStageFlagBits::eClosestHitKHR) //Textre Array
		.WithStorageBuffers(5, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //Material Layer Buffer
		.WithStorageBuffers(6, 1, vk::ShaderStageFlagBits::eClosestHitKHR) //SceneNode Buffer
		.Build("Vertex Buffer Descriptor Set Layout  Build.");

	BuildVertexBuffer(rtSceneBufferLayout, rtSceneBufferDescriptor, device, pool);
	SceneDesBufferBuild(device, pool);

	Vector2i windowSize = hostWindow.GetScreenSize();


	rayTexture = TextureBuilder(device, renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandBuffer::Graphics))
		.UsingQueue(renderer->GetQueue(CommandBuffer::Graphics))
		.WithDimension(windowSize.x, windowSize.y, 1)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eGeneral)
		.WithFormat(vk::Format::eB8G8R8A8Unorm)
	.Build("RaytraceResult");

	WriteStorageImageDescriptor(device, *imageDescriptor, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eGeneral);

	WriteBufferDescriptor(device , *inverseCamDescriptor, 0, vk::DescriptorType::eUniformBuffer, inverseMatrices);

	WriteTLASDescriptor(device , *rayTraceDescriptor, 0, *tlas);

	WriteImageDescriptor(device, *defaultSamplerDescriptor, 0, defaultTexture->GetDefaultView(), *defaultSampler);
	
	auto rtPipeBuilder = VulkanRayTracingPipelineBuilder(device)
		.WithRecursionDepth(1)
		.WithShader(*raygenShader, vk::ShaderStageFlagBits::eRaygenKHR)		//0
		.WithShader(*missShader, vk::ShaderStageFlagBits::eMissKHR)			//1
		.WithShader(*hitShader, vk::ShaderStageFlagBits::eClosestHitKHR)	//2
		.WithShader(*hitShader2, vk::ShaderStageFlagBits::eClosestHitKHR)	//3

		.WithGeneralGroup(0)	//Group for the raygen shader	//Uses shader 0
		.WithGeneralGroup(1)	//Group for the miss shader		//Uses shader 1
		.WithTriangleHitGroup(2)								//Uses shader 2
		.WithTriangleHitGroup(3)

		.WithDescriptorSetLayout(0, *rayTraceLayout)
		.WithDescriptorSetLayout(1, *cameraLayout)
		.WithDescriptorSetLayout(2, *inverseCamLayout)
		.WithDescriptorSetLayout(3, *imageLayout)
		.WithDescriptorSetLayout(4, *rtSceneBufferLayout)
		.WithDescriptorSetLayout(5, *dSamplerLayout);

	rtPipeline = rtPipeBuilder.Build("RT Pipeline");

	bindingTable = VulkanShaderBindingTableBuilder("SBT")
		.WithProperties(rayPipelineProperties)
		.WithPipeline(rtPipeline, rtPipeBuilder.GetCreateInfo())
		.Build(device, renderer->GetMemoryAllocator());

	//We also need some Vulkan things for displaying the result!
	displayImageLayout = DescriptorSetLayoutBuilder(device)
		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
		.Build("Raster Image Layout");

	displayImageDescriptor = CreateDescriptorSet(device , pool, *displayImageLayout);
	WriteImageDescriptor(device, *displayImageDescriptor, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);

	quadMesh = GenerateQuad();

	displayShader = ShaderBuilder(device)
		.WithVertexBinary("Display.vert.spv")
		.WithFragmentBinary("Display.frag.spv")
		.Build("Result Display Shader");

	displayPipeline = PipelineBuilder(device)
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(displayShader)
		.WithDescriptorSetLayout(0, *displayImageLayout)
	
		.WithColourAttachment(state.colourFormat)

		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat())
		.Build("Result display pipeline");
}

void NCL::Rendering::Vulkan::TestGLTFRayTrace::BuildTlas(const vk::Device& device)
{   
	for (const auto& mesh : scene.meshes) {
		VulkanMesh* vkMesh = (VulkanMesh*)mesh.get();
		bvhBuilder.WithObject(vkMesh, Matrix::Translation(Vector3{ 0,0,0 }) * Matrix::Scale(Vector3{ 1,1,1 }));
	}

	tlas = bvhBuilder
		.WithCommandQueue(renderer->GetQueue(CommandBuffer::AsyncCompute))
		.WithCommandPool(renderer->GetCommandPool(CommandBuffer::AsyncCompute))
		.WithDevice(device)
		.WithAllocator(renderer->GetMemoryAllocator())
		.Build(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, "GLTF BLAS");
}

TestGLTFRayTrace::~TestGLTFRayTrace() {
}

void TestGLTFRayTrace::RenderFrame(float dt) {
	hostWindow.SetTitle(std::to_string(1.0f / dt));

	Matrix4* inverseMatrixData = (Matrix4*)inverseMatrices.Data();

	inverseMatrixData[0] = Matrix::Inverse(camera.BuildViewMatrix());
	inverseMatrixData[1] = Matrix::Inverse(camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()));

	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);

	std::vector<vk::DescriptorSet> sets = {
		*rayTraceDescriptor,		//Set 0
		*cameraDescriptor,			//Set 1
		*inverseCamDescriptor,		//Set 2
		*imageDescriptor,			//Set 3
		*rtSceneBufferDescriptor,   //Set 4
		*dSamplerDescriptor			//Set 5
	};

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline.layout, 0, sets.size(), sets.data(), 0, nullptr);

	//Flip texture back for the next frame
	ImageTransitionBarrier(cmdBuffer, *rayTexture,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eGeneral,
		vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits2::eFragmentShader,
		vk::PipelineStageFlagBits2::eRayTracingShaderKHR);

	cmdBuffer.traceRaysKHR(
		&bindingTable.regions[BindingTableOrder::RayGen],
		&bindingTable.regions[BindingTableOrder::Miss],
		&bindingTable.regions[BindingTableOrder::Hit],
		&bindingTable.regions[BindingTableOrder::Call],
		frameState.defaultViewport.width, std::abs(frameState.defaultViewport.height), 1
	);

	//Flip the texture we rendered into into display mode
	ImageTransitionBarrier(cmdBuffer, *rayTexture,
		vk::ImageLayout::eGeneral,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
		vk::PipelineStageFlagBits2::eFragmentShader);

	//Now display the results on screen!
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, displayPipeline);

	renderer->BeginDefaultRendering(cmdBuffer);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *displayPipeline.layout, 0, 1, &*displayImageDescriptor, 0, nullptr);
	quadMesh->Draw(cmdBuffer);
	cmdBuffer.endRendering();
}

void NCL::Rendering::Vulkan::TestGLTFRayTrace::SceneDesBufferBuild(vk::Device& device, vk::DescriptorPool& pool)
{
	sceneDesBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(GLTFPrimInfo) * scene.primeInfoList.size(), "Scene Primitive Description Buffer Creation");

	sceneDesBuffer.CopyData(scene.primeInfoList.data(), sizeof(GLTFPrimInfo) * scene.primeInfoList.size());

	WriteBufferDescriptor(device, *rtSceneBufferDescriptor, 6, vk::DescriptorType::eStorageBuffer, sceneDesBuffer);
}

void NCL::Rendering::Vulkan::TestGLTFRayTrace::BuildVertexBuffer(vk::UniqueDescriptorSetLayout& inDesSetLayout, vk::UniqueDescriptorSet& outDesSet, vk::Device& device, vk::DescriptorPool& pool)
{
	std::vector<Vector3> posDataList;
	std::vector<unsigned int> indexList;
	std::vector<Vector2> texCoordDataList;
	std::vector<Vector3> normalDataList;
	std::vector<vk::DescriptorImageInfo> tempImageInfoList(scene.textures.size());
	std::vector<MaterialLayer> matLayerList;

	for (const auto& mesh : scene.meshes) 
	{
		VulkanMesh* vkMesh = (VulkanMesh*)mesh.get();
		posDataList.insert(posDataList.end(), vkMesh->GetPositionData().begin(), vkMesh->GetPositionData().end());
		indexList.insert(indexList.end(), vkMesh->GetIndexData().begin(), vkMesh->GetIndexData().end());
		texCoordDataList.insert(texCoordDataList.end(), vkMesh->GetTextureCoordData().begin(), vkMesh->GetTextureCoordData().end());
		normalDataList.insert(normalDataList.end(), vkMesh->GetNormalData().begin(), vkMesh->GetNormalData().end());
	}

	outDesSet = CreateDescriptorSet(device, pool, *inDesSetLayout);

	vertexPositionBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Vector3) * posDataList.size(), "Scene Vertex Position Buffer");
	vertexPositionBuffer.CopyData(posDataList.data(), sizeof(Vector3) * posDataList.size());

	indicesBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(unsigned int) * indexList.size(), "Scene Vertex Index Buffer");
	indicesBuffer.CopyData(indexList.data(), sizeof(unsigned int) * indexList.size());

	texCordBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Vector2) * texCoordDataList.size(), "Scene Vertex Tex-coord Buffer");
	texCordBuffer.CopyData(texCoordDataList.data(), sizeof(Vector2) * texCoordDataList.size());

	vertexNormalBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Vector3) * normalDataList.size(), "Scene Vertex Normal Buffer");
	vertexNormalBuffer.CopyData(normalDataList.data(), sizeof(Vector3) * normalDataList.size());

	WriteBufferDescriptor(device, *outDesSet, 0, vk::DescriptorType::eStorageBuffer, vertexPositionBuffer);
	WriteBufferDescriptor(device, *outDesSet, 1, vk::DescriptorType::eStorageBuffer, indicesBuffer);
	WriteBufferDescriptor(device, *outDesSet, 2, vk::DescriptorType::eStorageBuffer, texCordBuffer);
	WriteBufferDescriptor(device, *outDesSet, 3, vk::DescriptorType::eStorageBuffer, vertexNormalBuffer);


	for (size_t i = 0; i < scene.textures.size(); i++)
	{
		tempImageInfoList[i].sampler = defaultSampler.get();
		tempImageInfoList[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		std::shared_ptr<VulkanTexture> tempVkTex = std::dynamic_pointer_cast<VulkanTexture>(scene.textures[i]);
		tempImageInfoList[i].imageView = tempVkTex.get()->GetDefaultView();
	}

	vk::WriteDescriptorSet tempDesSet{
		.dstSet = outDesSet.get(),
		.dstBinding = 4,
		.dstArrayElement = 0,
		.descriptorCount = static_cast<uint32_t>(scene.textures.size()),
		.descriptorType = vk::DescriptorType::eSampledImage
	};

	WriteDescriptor(device, tempDesSet, tempImageInfoList);

	for (const auto& i : scene.materialLayers)
		matLayerList.emplace_back(MaterialLayer(i));

	matLayerBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(MaterialLayer) * matLayerList.size(), "Material Buffer");
	matLayerBuffer.CopyData(matLayerList.data(), sizeof(MaterialLayer) * matLayerList.size());
	WriteBufferDescriptor(device, *outDesSet, 5, vk::DescriptorType::eStorageBuffer, matLayerBuffer);
}