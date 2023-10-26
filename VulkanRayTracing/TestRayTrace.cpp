/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "TestRayTrace.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TestRayTrace::TestRayTrace(Window& window) : VulkanTutorialRenderer(window){
	majorVersion = 1;
	minorVersion = 3;

	autoBeginDynamicRendering = false;
}

TestRayTrace::~TestRayTrace() {
}

void TestRayTrace::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
	deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

	static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures(true);

	static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures(true);
	rayFeatures.setRayTracingPipeline(true);

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature(true);

	deviceFeatures.setPNext(&accelFeatures);
	accelFeatures.pNext = &rayFeatures;
	rayFeatures.pNext	= &deviceAddressfeature;
	
	allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
}

void TestRayTrace::SetupTutorial() {
	VulkanTutorialRenderer::SetupTutorial();

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &rayPipelineProperties;

	GetPhysicalDevice().getProperties2(&props);

	triangle = GenerateTriangle();

	raygenShader	= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/raygen.rgen.spv", GetDevice())); 
	hitShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/closesthit.rchit.spv", GetDevice())); 
	missShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/miss.rmiss.spv", GetDevice()));

	rayTraceLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithAccelStructures(1)
		.Build("Ray Trace TLAS Layout");

	imageLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithStorageImages(1)
		.Build("Ray Trace Image Layout");

	inverseCamLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithUniformBuffers(1)
		.Build("Camera Inverse Matrix Layout");

	tlas = bvhBuilder
		.WithObject(&*triangle, Matrix4::Translation({ 0,0,-100.0f }) * Matrix4::Scale({2,4,2}))
		.WithCommandQueue(GetQueue(CommandBuffer::AsyncCompute))
		.WithCommandPool(GetCommandPool(CommandBuffer::AsyncCompute)) 
		.WithDevice(GetDevice())
		.WithAllocator(GetMemoryAllocator())
		.Build(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, "Test TLAS");

	rayTraceDescriptor		= BuildUniqueDescriptorSet(*rayTraceLayout);
	imageDescriptor			= BuildUniqueDescriptorSet(*imageLayout);
	inverseCamDescriptor	= BuildUniqueDescriptorSet(*inverseCamLayout);

	inverseMatrices = BufferBuilder(GetDevice(), GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * 2, "InverseMatrices");

	//rayTexture = VulkanTexture::CreateColourTexture(this, windowSize.x, windowSize.y, "RayTraceResult", 
	//	vk::Format::eR32G32B32A32Sfloat, 
	//	vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
	//	vk::ImageLayout::eGeneral
	//);

	rayTexture = TextureBuilder(GetDevice(), GetMemoryAllocator())
		.WithPool(GetCommandPool(CommandBuffer::Graphics))
		.WithQueue(GetQueue(CommandBuffer::Graphics))
		.WithDimension(windowSize.x, windowSize.y, 1)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
		.WithPipeFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eGeneral)
		.WithFormat(vk::Format::eB8G8R8A8Unorm)
		.Build("RaytraceResult");


	WriteStorageImageDescriptor(*imageDescriptor, 0, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eGeneral);

	WriteBufferDescriptor(*inverseCamDescriptor, 0, vk::DescriptorType::eUniformBuffer, inverseMatrices);

	WriteTLASDescriptor(*rayTraceDescriptor, 0, *tlas);

	auto rtPipeBuilder = VulkanRayTracingPipelineBuilder(GetDevice())
		.WithRecursionDepth(1)
		.WithShader(*raygenShader, vk::ShaderStageFlagBits::eRaygenKHR)		//0
		.WithShader(*missShader, vk::ShaderStageFlagBits::eMissKHR)			//1
		.WithShader(*hitShader, vk::ShaderStageFlagBits::eClosestHitKHR)	//2

		.WithGeneralGroup(0)	//Group for the raygen shader	//Uses shader 0
		.WithGeneralGroup(1)	//Group for the miss shader		//Uses shader 1
		.WithTriangleHitGroup(2)								//Uses shader 2

		.WithDescriptorSetLayout(0, *rayTraceLayout)
		.WithDescriptorSetLayout(1, *cameraLayout)
		.WithDescriptorSetLayout(2, *inverseCamLayout)
		.WithDescriptorSetLayout(3, *imageLayout);

	rtPipeline = rtPipeBuilder.Build("RT Pipeline");

	bindingTable = VulkanShaderBindingTableBuilder("SBT")
		.WithProperties(rayPipelineProperties)
		.WithPipeline(rtPipeline, rtPipeBuilder.GetCreateInfo())
		.Build(GetDevice(), GetMemoryAllocator());

	//We also need some Vulkan things for displaying the result!
	displayImageLayout = DescriptorSetLayoutBuilder(GetDevice())
		.WithSamplers(1, vk::ShaderStageFlagBits::eFragment)
		.Build("Raster Image Layout");

	displayImageDescriptor = BuildUniqueDescriptorSet(*displayImageLayout);
	WriteImageDescriptor(*displayImageDescriptor, 0, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);

	quadMesh = GenerateQuad();

	displayShader = ShaderBuilder(GetDevice())
		.WithVertexBinary("Display.vert.spv")
		.WithFragmentBinary("Display.frag.spv")
		.Build("Result Display Shader");

	displayPipeline = PipelineBuilder(GetDevice())
		.WithVertexInputState(quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShader(displayShader)
		.WithDescriptorSetLayout(0, *displayImageLayout)
		.WithColourAttachment(GetSurfaceFormat())
		.WithDepthAttachment(depthBuffer->GetFormat())
		.Build("Result display pipeline");
}

void TestRayTrace::RenderFrame() {
	frameCmds.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);

	vk::DescriptorSet sets[4] = {
		*rayTraceDescriptor,		//Set 0
		*cameraDescriptor,			//Set 1
		*inverseCamDescriptor,		//Set 2
		*imageDescriptor			//Set 3
	};

	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline.layout, 0, 4, sets, 0, nullptr);

	frameCmds.traceRaysKHR(
		&bindingTable.regions[BindingTableOrder::RayGen],
		&bindingTable.regions[BindingTableOrder::Miss],
		&bindingTable.regions[BindingTableOrder::Hit],
		&bindingTable.regions[BindingTableOrder::Call],
		windowSize.x, windowSize.y, 1
	);

	//Flip the texture we rendered into into display mode
	ImageTransitionBarrier(frameCmds, *rayTexture,
		vk::ImageLayout::eGeneral,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits::eRayTracingShaderKHR,
		vk::PipelineStageFlagBits::eFragmentShader);

	//Now display the results on screen!
	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, displayPipeline);

	BeginDefaultRendering(frameCmds);
	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *displayPipeline.layout, 0, 1, &*displayImageDescriptor, 0, nullptr);
	DrawMesh(frameCmds, *quadMesh);
	frameCmds.endRendering();

	//Flip texture back for the next frame
	ImageTransitionBarrier(frameCmds, *rayTexture,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eGeneral,
		vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::PipelineStageFlagBits::eRayTracingShaderKHR);
}

void TestRayTrace::Update(float dt) {
	VulkanTutorialRenderer::Update(dt);

	Matrix4* inverseMatrixData = (Matrix4*)inverseMatrices.Data();

	inverseMatrixData[0] = camera.BuildViewMatrix().Inverse();
	inverseMatrixData[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()).Inverse();
}
