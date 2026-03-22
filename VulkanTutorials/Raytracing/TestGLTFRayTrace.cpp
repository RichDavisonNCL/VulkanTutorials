///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
#include "TestGLTFRayTrace.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include "../GLTFLoader/GLTFLoader.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;
#if USE_IMGUI
using namespace Win32Code;
#endif // USE_IMGUI

TUTORIAL_ENTRY(TestGLTFRayTrace)

TestGLTFRayTrace::TestGLTFRayTrace(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit) {
	m_vkInit.majorVersion = 1;
	m_vkInit.minorVersion = 3;
	m_vkInit.autoBeginDynamicRendering = false;

	m_vkInit.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	m_vkInit.deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	m_vkInit.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	m_vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	m_vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	m_vkInit.defaultDescriptorPoolAccelerationStructureCount = 128;

	static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
		.accelerationStructure = true
	};

	static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures = {
		.rayTracingPipeline = true
	};

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
	};

	m_vkInit.features.push_back((void*)&accelFeatures);
	m_vkInit.features.push_back((void*)&rayFeatures);
	m_vkInit.features.push_back((void*)&deviceAddressfeature);

	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	GLTFLoader::Load("Sponza/Sponza.gltf",scene);

	GLTFLoader::Load("CesiumMan/CesiumMan.gltf",scene);

	for (const auto& m : scene.meshes) {
		VulkanMesh* loadedMesh = (VulkanMesh*)m.get();
		UploadMeshWait(*loadedMesh, vk::BufferUsageFlagBits::eShaderDeviceAddress | 
									vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
	}

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &rayPipelineProperties;
	m_renderer->GetPhysicalDevice().getProperties2(&props);

	rayTraceLayout = DescriptorSetLayoutBuilder(context.device)
		.WithAccelStructures(0, 1)
		.Build("Ray Trace TLAS Layout");

	imageLayout = DescriptorSetLayoutBuilder(context.device)
		.WithStorageImages(0, 1)
		.Build("Ray Trace Image Layout");

	inverseCamLayout = DescriptorSetLayoutBuilder(context.device)
		.WithUniformBuffers(0, 1)
		.Build("Camera Inverse Matrix Layout");

	for (const auto& n : scene.sceneNodes) {
		if (n.mesh == nullptr) {
			continue;
		}
		VulkanMesh* vm = (VulkanMesh*)n.mesh.get();
		bvhBuilder.WithObject(vm, n.worldMatrix);
	}

	tlas = bvhBuilder
		.WithCommandQueue(context.queues[CommandType::AsyncCompute])
		.WithCommandPool(context.commandPools[CommandType::AsyncCompute])
		.WithDevice(context.device)
		.WithAllocator(*m_memoryManager)
		.Build(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, "GLTF BLAS");

	rayTraceDescriptor		= CreateDescriptorSet(context.device, context.descriptorPool, *rayTraceLayout);
	imageDescriptor			= CreateDescriptorSet(context.device, context.descriptorPool, *imageLayout);
	inverseCamDescriptor	= CreateDescriptorSet(context.device, context.descriptorPool, *inverseCamLayout);

	inverseMatrices = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Matrix4) * 2,
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer
		},
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		"Inverse Matrices"
	);

	Vector2i windowSize = m_hostWindow.GetScreenSize();

	vk::UniqueCommandBuffer cmdBuffer = Vulkan::CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics]);

	rayTexture = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(*cmdBuffer)
		.WithDimension(windowSize.x, windowSize.y, 1)
		.WithUsages(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
		.WithPipeFlags(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.WithLayout(vk::ImageLayout::eGeneral)
		.WithFormat(vk::Format::eR32G32B32A32Sfloat)
	.Build("RaytraceResult");

	Vulkan::CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	WriteStorageImageDescriptor(context.device, *imageDescriptor, 0, *rayTexture, *m_defaultSampler, vk::ImageLayout::eGeneral);

	WriteBufferDescriptor(context.device, *inverseCamDescriptor, 0, vk::DescriptorType::eUniformBuffer, inverseMatrices);

	WriteTLASDescriptor(context.device, *rayTraceDescriptor, 0, *tlas);

	auto rtPipeBuilder = VulkanRayTracingPipelineBuilder(context.device);
	rtPipeBuilder.WithRecursionDepth(1)
		.WithShaderBinary("RayTrace/raygen.rgen.spv"		, vk::ShaderStageFlagBits::eRaygenKHR)		//0
		.WithShaderBinary("RayTrace/miss.rmiss.spv"			, vk::ShaderStageFlagBits::eMissKHR)		//1
		.WithShaderBinary("RayTrace/closesthit.rchit.spv"	, vk::ShaderStageFlagBits::eClosestHitKHR)	//2

		.WithRayGenGroup(0)	//Group for the raygen shader	//Group 0 Uses shader 0
		.WithMissGroup(1)	//Group for the miss shader		//Group 1 Uses shader 1
		.WithTriangleHitGroup(2)	//Hit group 0			//Group 2 Uses shader 2

		.WithDescriptorSetLayout(0, rayTraceLayout)
		.WithDescriptorSetLayout(1, m_cameraLayout)
		.WithDescriptorSetLayout(2, inverseCamLayout)
		.WithDescriptorSetLayout(3, imageLayout);

	rtPipeline = rtPipeBuilder.Build("RT Pipeline");

	bindingTable = VulkanShaderBindingTableBuilder("SBT")
		.WithProperties(rayPipelineProperties)
		.WithPipeline(rtPipeline, rtPipeBuilder.GetCreateInfo())
		.Build(context.device, *m_memoryManager);

	//We also need some Vulkan things for displaying the result!
	displayImageLayout = DescriptorSetLayoutBuilder(context.device)
		.WithImageSamplers(0, 1, vk::ShaderStageFlagBits::eFragment)
		.Build("Raster Image Layout");

	displayImageDescriptor = CreateDescriptorSet(context.device, context.descriptorPool, *displayImageLayout);
	WriteCombinedImageDescriptor(context.device, *displayImageDescriptor, 0, *rayTexture, *m_defaultSampler, vk::ImageLayout::eShaderReadOnlyOptimal);

	displayPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_quadMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleStrip)
		.WithShaderBinary("Display.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("Display.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDescriptorSetLayout(0, *displayImageLayout)
	
		.WithColourAttachment(context.colourFormat)

		.WithDepthAttachment(context.depthFormat)
		.Build("Result display pipeline");

#if USE_IMGUI
	Win32Window* windowWrapper = (Win32Window*)&m_hostWindow;
	ui.Init(windowWrapper->GetHandle(), m_renderer);
#endif // USE_IMGUI
}

TestGLTFRayTrace::~TestGLTFRayTrace() {
#if USE_IMGUI
	ui.Destroy();
#endif // USE_IMGUI
}

void TestGLTFRayTrace::RenderFrame(float dt) {
#if USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, m_hostWindow.GetMouse()->ButtonDown(NCL::MouseButtons::Left));
#endif // USE_IMGUI

	Matrix4* inverseMatrixData = inverseMatrices.Map<Matrix4>();

	inverseMatrixData[0] = Matrix::Inverse(m_camera.BuildViewMatrix());
	inverseMatrixData[1] = Matrix::Inverse(m_camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect()));

	inverseMatrices.Unmap();

	FrameContext const& context = m_renderer->GetFrameContext();
	vk::CommandBuffer cmdBuffer = context.cmdBuffer;

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);

	vk::DescriptorSet sets[4] = {
		*rayTraceDescriptor,		//Set 0
		*m_cameraDescriptor,			//Set 1
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
		context.viewport.width, std::abs(context.viewport.height), 1
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

	m_renderer->BeginRenderToScreen(cmdBuffer);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *displayPipeline.layout, 0, 1, &*displayImageDescriptor, 0, nullptr);
	m_quadMesh->Draw(cmdBuffer);

#if USE_IMGUI
	ui.StartNewFrame();

    // Test
	bool updateAccumulation = false;
	bool showWindow = true;
	
	ImGui::SetWindowPos(ImVec2(1.0f, 1.0f));
	ImGui::Begin("Settings", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("test text.");

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Generic:", ImGuiTreeNodeFlags_DefaultOpen)) {
		
		ImGui::Indent(12.0f);
		updateAccumulation |= ImGui::Checkbox("Demo Window", &showWindow); ImGui::SameLine();
		ImGui::Indent(-12.0f);
	}
	
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Path Tracing:", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent(12.0f);
		updateAccumulation |= ImGui::Combo("Debug Output", (int*)&ptDebugOutput, ptDebugOutputTypeStrings);
		ImGui::Indent(-12.0f);
	}

	ImGui::Separator();

	ImGui::End();

	ui.Render(cmdBuffer);
#endif // USE_IMGUI

	cmdBuffer.endRendering();
}


void TestGLTFRayTrace::Update(float dt)
{
	m_runTime += dt;
	if (m_hostWindow.GetMouse()->ButtonDown(NCL::MouseButtons::Left)) {
		UpdateCamera(dt);
	}

	UploadCameraUniform();

	m_renderer->Update(dt);
}