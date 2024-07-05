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

TUTORIAL_ENTRY(TestGLTFRayTrace)

TestGLTFRayTrace::TestGLTFRayTrace(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window) {
	vkInit.majorVersion = 1;
	vkInit.minorVersion = 3;
	vkInit.autoBeginDynamicRendering = false;
	vkInit.skipDynamicState = true;

	vkInit.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
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

	vkInit.features.push_back((void*)&accelFeatures);
	vkInit.features.push_back((void*)&rayFeatures);
	vkInit.features.push_back((void*)&deviceAddressfeature);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& state = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	GLTFLoader::Load("Sponza/Sponza.gltf",scene);

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		loadedMesh->UploadToGPU(renderer,	vk::BufferUsageFlagBits::eShaderDeviceAddress | 
										vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
	}

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &rayPipelineProperties;
	renderer->GetPhysicalDevice().getProperties2(&props);

	raygenShader	= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/raygen.rgen.spv", device));
	hitShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/closesthit.rchit.spv", device));
	missShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/miss.rmiss.spv", device));

	rayTraceLayout = DescriptorSetLayoutBuilder(device)
		.WithAccelStructures(0, 1)
		.Build("Ray Trace TLAS Layout");

	imageLayout = DescriptorSetLayoutBuilder(device)
		.WithStorageImages(0, 1)
		.Build("Ray Trace Image Layout");

	inverseCamLayout = DescriptorSetLayoutBuilder(device)
		.WithUniformBuffers(0, 1)
		.Build("Camera Inverse Matrix Layout");

	for (const auto& n : scene.sceneNodes) {
		if (n.mesh == nullptr) {
			continue;
		}
		VulkanMesh* vm = (VulkanMesh*)n.mesh;
		bvhBuilder.WithObject(vm, n.worldMatrix);
	}
	 
	tlas = bvhBuilder
		.WithCommandQueue(renderer->GetQueue(CommandType::AsyncCompute))
		.WithCommandPool(renderer->GetCommandPool(CommandType::AsyncCompute)) 
		.WithDevice(device)
		.WithAllocator(renderer->GetMemoryAllocator())
		.Build(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, "GLTF BLAS");

	rayTraceDescriptor		= CreateDescriptorSet(device, pool, *rayTraceLayout);
	imageDescriptor			= CreateDescriptorSet(device, pool, *imageLayout);
	inverseCamDescriptor	= CreateDescriptorSet(device, pool, *inverseCamLayout);

	inverseMatrices = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * 2, "InverseMatrices");

	Vector2i windowSize = hostWindow.GetScreenSize();

	rayTexture = TextureBuilder(device, renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandType::Graphics))
		.UsingQueue(renderer->GetQueue(CommandType::Graphics))
		.WithDimension(windowSize.x, windowSize.y, 1)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eGeneral)
		.WithFormat(vk::Format::eB8G8R8A8Unorm)
	.Build("RaytraceResult");

	WriteStorageImageDescriptor(device, *imageDescriptor, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eGeneral);

	WriteBufferDescriptor(device , *inverseCamDescriptor, 0, vk::DescriptorType::eUniformBuffer, inverseMatrices);

	WriteTLASDescriptor(device , *rayTraceDescriptor, 0, *tlas);

	auto rtPipeBuilder = VulkanRayTracingPipelineBuilder(device)
		.WithRecursionDepth(1)

		.WithShader(*raygenShader, vk::ShaderStageFlagBits::eRaygenKHR)		//0
		.WithShader(*missShader, vk::ShaderStageFlagBits::eMissKHR)			//1
		.WithShader(*hitShader, vk::ShaderStageFlagBits::eClosestHitKHR)	//2

		.WithRayGenGroup(0)	//Group for the raygen shader	//Uses shader 0
		.WithMissGroup(1)	//Group for the miss shader		//Uses shader 1
		.WithTriangleHitGroup(2)	//Hit group 0			//Uses shader 2

		.WithDescriptorSetLayout(0, *rayTraceLayout)
		.WithDescriptorSetLayout(1, *cameraLayout)
		.WithDescriptorSetLayout(2, *inverseCamLayout)
		.WithDescriptorSetLayout(3, *imageLayout);

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

TestGLTFRayTrace::~TestGLTFRayTrace() {
}

void TestGLTFRayTrace::RenderFrame(float dt) {
	Matrix4* inverseMatrixData = (Matrix4*)inverseMatrices.Data();

	inverseMatrixData[0] = Matrix::Inverse(camera.BuildViewMatrix());
	inverseMatrixData[1] = Matrix::Inverse(camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()));

	FrameState const& frameState = renderer->GetFrameState();
	vk::CommandBuffer cmdBuffer = frameState.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);

	vk::DescriptorSet sets[4] = {
		*rayTraceDescriptor,		//Set 0
		*cameraDescriptor,			//Set 1
		*inverseCamDescriptor,		//Set 2
		*imageDescriptor			//Set 3
	};

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline.layout, 0, 4, sets, 0, nullptr);

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