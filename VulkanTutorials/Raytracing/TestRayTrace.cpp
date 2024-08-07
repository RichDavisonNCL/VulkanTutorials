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

//TestRayTrace::TestRayTrace(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window){
//	autoBeginDynamicRendering = false;
//}
//
//TestRayTrace::~TestRayTrace() {
//}
//
////void TestRayTrace::SetupDevice(vk::PhysicalDeviceFeatures2& deviceFeatures) {
////	//static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
////	//	.accelerationStructure = true
////	//};
////
////	//static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures = {
////	//	.rayTracingPipeline = true
////	//};
////
////	//static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
////	//	.bufferDeviceAddress = true
////	//};
////
////	//deviceFeatures.setPNext(&accelFeatures);
////	//accelFeatures.pNext = &rayFeatures;
////	//rayFeatures.pNext	= &deviceAddressfeature;
////	//
////	//allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
////}
//
//void TestRayTrace::SetupTutorial() {
//	vk::Device device = renderer->GetDevice();
//
//	
//	
//
//	VulkanTutorial::SetupTutorial();
//
//	vk::PhysicalDeviceProperties2 props;
//	props.pNext = &rayPipelineProperties;
//
//	renderer->GetPhysicalDevice().getProperties2(&props);
//
//	triangle = GenerateTriangle();
//
//	raygenShader	= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/raygen.rgen.spv", device));
//	hitShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/closesthit.rchit.spv", device));
//	missShader		= UniqueVulkanRTShader(new VulkanRTShader("RayTrace/miss.rmiss.spv", device));
//
//	rayTraceLayout = DescriptorSetLayoutBuilder(device)
//		.WithAccelStructures(0, 1)
//		.Build("Ray Trace TLAS Layout");
//
//	imageLayout = DescriptorSetLayoutBuilder(device)
//		.WithStorageImages(0, 1)
//		.Build("Ray Trace Image Layout");
//
//	inverseCamLayout = DescriptorSetLayoutBuilder(device)
//		.WithUniformBuffers(0, 1)
//		.Build("Camera Inverse Matrix Layout");
//
//	tlas = bvhBuilder
//		.WithObject(&*triangle, Matrix::Translation(Vector3{ 0,0,-100.0f }) * Matrix::Scale(Vector3{2,4,2}))
//		.WithCommandQueue(renderer->GetQueue(CommandBuffer::AsyncCompute))
//		.WithCommandPool(renderer->GetCommandPool(CommandBuffer::AsyncCompute))
//		.WithDevice(device)
//		.WithAllocator(renderer->GetMemoryAllocator())
//		.Build(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, "Test TLAS");
//
//	rayTraceDescriptor		= CreateDescriptorSet(device, *rayTraceLayout);
//	imageDescriptor			= CreateDescriptorSet(device, *imageLayout);
//	inverseCamDescriptor	= CreateDescriptorSet(device, *inverseCamLayout);
//
//	inverseMatrices = BufferBuilder(device, renderer->GetMemoryAllocator())
//		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
//		.WithHostVisibility()
//		.WithPersistentMapping()
//		.Build(sizeof(Matrix4) * 2, "InverseMatrices");
//
//	rayTexture = TextureBuilder(device, renderer->GetMemoryAllocator())
//		.UsingPool(renderer->GetCommandPool(CommandBuffer::Graphics))
//		.UsingQueue(renderer->GetQueue(CommandBuffer::Graphics))
//		.WithDimension(windowSize.x, windowSize.y, 1)
//		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
//		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
//		.WithLayout(vk::ImageLayout::eGeneral)
//		.WithFormat(vk::Format::eB8G8R8A8Unorm)
//		.Build("RaytraceResult");
//
//	WriteStorageImageDescriptor(device , *imageDescriptor, 0, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eGeneral);
//
//	WriteBufferDescriptor(device , *inverseCamDescriptor, 0, vk::DescriptorType::eUniformBuffer, inverseMatrices);
//
//	WriteTLASDescriptor(device , *rayTraceDescriptor, 0, *tlas);
//
//	auto rtPipeBuilder = VulkanRayTracingPipelineBuilder(device)
//		.WithRecursionDepth(1)
//		.WithShader(*raygenShader, vk::ShaderStageFlagBits::eRaygenKHR)		//0
//		.WithShader(*missShader, vk::ShaderStageFlagBits::eMissKHR)			//1
//		.WithShader(*hitShader, vk::ShaderStageFlagBits::eClosestHitKHR)	//2
//
//		.WithGeneralGroup(0)	//Group for the raygen shader	//Uses shader 0
//		.WithGeneralGroup(1)	//Group for the miss shader		//Uses shader 1
//		.WithTriangleHitGroup(2)								//Uses shader 2
//
//		.WithDescriptorSetLayout(0, *rayTraceLayout)
//		.WithDescriptorSetLayout(1, *cameraLayout)
//		.WithDescriptorSetLayout(2, *inverseCamLayout)
//		.WithDescriptorSetLayout(3, *imageLayout);
//
//	rtPipeline = rtPipeBuilder.Build("RT Pipeline");
//
//	bindingTable = VulkanShaderBindingTableBuilder("SBT")
//		.WithProperties(rayPipelineProperties)
//		.WithPipeline(rtPipeline, rtPipeBuilder.GetCreateInfo())
//		.Build(device, renderer->GetMemoryAllocator());
//
//	//We also need some Vulkan things for displaying the result!
//	displayImageLayout = DescriptorSetLayoutBuilder(device)
//		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
//		.Build("Raster Image Layout");
//
//	displayImageDescriptor = CreateDescriptorSet(*displayImageLayout);
//	WriteImageDescriptor(*displayImageDescriptor, 0, 0, *rayTexture, *defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);
//
//	quadMesh = GenerateQuad();
//
//	displayShader = ShaderBuilder(device)
//		.WithVertexBinary("Display.vert.spv")
//		.WithFragmentBinary("Display.frag.spv")
//		.Build("Result Display Shader");
//
//	displayPipeline = PipelineBuilder(device)
//		.WithVertexInputState(quadMesh->GetVertexInputState())
//		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
//		.WithShader(displayShader)
//		.WithDescriptorSetLayout(0, *displayImageLayout)
//		.WithColourAttachment(renderer->GetSurfaceFormat())
//		.WithDepthAttachment(renderer->GetDepthBuffer()->GetFormat())
//		.Build("Result display pipeline");
//}
//
//void TestRayTrace::RenderFrame() {
//	frameCmds.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);
//
//	vk::DescriptorSet sets[4] = {
//		*rayTraceDescriptor,		//Set 0
//		*cameraDescriptor,			//Set 1
//		*inverseCamDescriptor,		//Set 2
//		*imageDescriptor			//Set 3
//	};
//
//	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline.layout, 0, 4, sets, 0, nullptr);
//
//	frameCmds.traceRaysKHR(
//		&bindingTable.regions[BindingTableOrder::RayGen],
//		&bindingTable.regions[BindingTableOrder::Miss],
//		&bindingTable.regions[BindingTableOrder::Hit],
//		&bindingTable.regions[BindingTableOrder::Call],
//		windowSize.x, windowSize.y, 1
//	);
//
//	//Flip the texture we rendered into into display mode
//	ImageTransitionBarrier(frameCmds, *rayTexture,
//		vk::ImageLayout::eGeneral,
//		vk::ImageLayout::eShaderReadOnlyOptimal,
//		vk::ImageAspectFlagBits::eColor,
//		vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
//		vk::PipelineStageFlagBits2::eFragmentShader);
//
//	//Now display the results on screen!
//	frameCmds.bindPipeline(vk::PipelineBindPoint::eGraphics, displayPipeline);
//
//	BeginDefaultRendering(frameCmds);
//	frameCmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *displayPipeline.layout, 0, 1, &*displayImageDescriptor, 0, nullptr);
//	quadMesh->Draw(frameCmds);
//	frameCmds.endRendering();
//
//	//Flip texture back for the next frame
//	ImageTransitionBarrier(frameCmds, *rayTexture,
//		vk::ImageLayout::eShaderReadOnlyOptimal,
//		vk::ImageLayout::eGeneral,
//		vk::ImageAspectFlagBits::eColor,
//		vk::PipelineStageFlagBits2::eFragmentShader,
//		vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
//}
//
//void TestRayTrace::Update(float dt) {
//	VulkanTutorial::Update(dt);
//
//	Matrix4* inverseMatrixData = (Matrix4*)inverseMatrices.Data();
//
//	inverseMatrixData[0] = Matrix::Inverse(camera.BuildViewMatrix());
//	inverseMatrixData[1] = Matrix::Inverse(camera.BuildProjectionMatrix(hostWindow.GetScreenAspect()));
//}
