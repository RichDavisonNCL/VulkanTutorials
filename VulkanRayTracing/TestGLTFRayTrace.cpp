/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "TestGLTFRayTrace.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"

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
		.accelerationStructure = true
	};

	static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures = {
		.rayTracingPipeline = true
	};

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
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

	GLTFLoader::Load("Sponza/Sponza.gltf",scene);

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);
	GLTFLoader::Load("DamagedHelmet/DamagedHelmet.gltf", scene);

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

	BuildVertexBuffer<Vector3>(vertexPositionBufferLayout, vertexPositionBufferDescriptor, vertexPositionBuffer,
		device,pool, VertexAttribute::Positions, "Scene Vertex Position Buffer Layout", "Scene Vertex Position Buffer");
	BuildVertexBuffer<Vector2>(texCordBufferLayout, texCordBufferDescriptor, texCordBuffer,
		device, pool, VertexAttribute::TextureCoords, "Scene Tex Coord Buffer Layout", "Scene Tex Coord Buffer");
	BuildVertexIndexBuffer(indicesBufferLayout, indicesBufferDescriptor, indicesBuffer,
		device, pool, "Scene indices Buffer Layout", "Scene indeces Buffer");

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
		.WithDescriptorSetLayout(4, *defaultSamplerLayout)
		.WithDescriptorSetLayout(5, *vertexPositionBufferLayout)
		.WithDescriptorSetLayout(6, *texCordBufferLayout)
		.WithDescriptorSetLayout(7, *indicesBufferLayout);

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

	vk::DescriptorSet sets[8] = {
		*rayTraceDescriptor,		//Set 0
		*cameraDescriptor,			//Set 1
		*inverseCamDescriptor,		//Set 2
		*imageDescriptor,			//Set 3
		*defaultSamplerDescriptor,  //Set 4
		*vertexPositionBufferDescriptor,    //Set 5
		*texCordBufferDescriptor, //Set 6
		*indicesBufferDescriptor //Set7
	};

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline.layout, 0, 8, sets, 0, nullptr);

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

void NCL::Rendering::Vulkan::TestGLTFRayTrace::BuildVertexIndexBuffer(vk::UniqueDescriptorSetLayout& outDesSetLayout, vk::UniqueDescriptorSet& outDesSet, VulkanBuffer& outBuffer, vk::Device& device, vk::DescriptorPool& pool, const std::string& inDescriptorSetLayoutDebugName, const std::string& inBufferDebugName)
{
	std::vector<unsigned int> vertexDataList;

	for (const auto& mesh : scene.meshes)
	{
		VulkanMesh* vkMesh = (VulkanMesh*)mesh.get();
		vertexDataList.insert(vertexDataList.end(), vkMesh->GetIndexData().begin(), vkMesh->GetIndexData().end());
	}

	outDesSetLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eClosestHitKHR)
		.Build(inDescriptorSetLayoutDebugName);

	outDesSet = CreateDescriptorSet(device, pool, *outDesSetLayout);

	outBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(unsigned int) * vertexDataList.size(), inBufferDebugName);

	outBuffer.CopyData(vertexDataList.data(), sizeof(unsigned int) * vertexDataList.size());

	WriteBufferDescriptor(device, *outDesSet, 1, vk::DescriptorType::eStorageBuffer, outBuffer);
}

template <typename T>
void NCL::Rendering::Vulkan::TestGLTFRayTrace::BuildVertexBuffer(vk::UniqueDescriptorSetLayout& outDesSetLayout,
	vk::UniqueDescriptorSet& outDesSet, VulkanBuffer& outBuffer,vk::Device& device, vk::DescriptorPool& pool,
	VertexAttribute::Type inAttribute, const std::string& inDescriptorSetLayoutDebugName, const std::string& inBufferDebugName)
{
	std::vector<T> vertexDataList;

	for (const auto& mesh : scene.meshes) 
	{
		VulkanMesh* vkMesh = (VulkanMesh*)mesh.get();
		
		if (inAttribute == VertexAttribute::Type::Positions)
			vertexDataList.insert(vertexDataList.end(), vkMesh->GetPositionData().begin(), vkMesh->GetPositionData().end());
		if (inAttribute == VertexAttribute::Type::Normals)
			vertexDataList.insert(vertexDataList.end(), vkMesh->GetNormalData().begin(), vkMesh->GetNormalData().end());
		if (inAttribute == VertexAttribute::Type::TextureCoords)
			vertexDataList.insert(vertexDataList.end(), vkMesh->GetTextureCoordData().begin(), vkMesh->GetTextureCoordData().end());
	}

	outDesSetLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eClosestHitKHR)
		.Build(inDescriptorSetLayoutDebugName);

	outDesSet = CreateDescriptorSet(device, pool, *outDesSetLayout);

	outBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(T) * vertexDataList.size(), inBufferDebugName);

	outBuffer.CopyData(vertexDataList.data(), sizeof(T) * vertexDataList.size());

	WriteBufferDescriptor(device, *outDesSet, 1, vk::DescriptorType::eStorageBuffer, outBuffer);
}