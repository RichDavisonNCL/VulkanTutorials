/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanTutorial.h"
#include "MshLoader.h"
#include "GLTFLoader.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanTutorial::VulkanTutorial(Window& window) : hostWindow(window), controller(*window.GetKeyboard(), *window.GetMouse()) {
	runTime			= 0.0f;

	GLTFLoader::SetMeshConstructionFunction(
		[&]()-> std::shared_ptr<Mesh> {return std::shared_ptr<Mesh> (new VulkanMesh()); }
	);

	GLTFLoader::SetTextureConstructionFunction(
		[&](std::string& input) ->  SharedTexture {return std::shared_ptr<Texture>(LoadTexture(input).release()); }
	);
}

VulkanTutorial::~VulkanTutorial() {
}

void VulkanTutorial::InitTutorialObjects() {
	BuildCamera();

	vk::Device device = renderer->GetDevice();

	defaultSampler = device.createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(16)
		.setMinFilter(vk::Filter::eLinear)
		.setMagFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMaxLod(80.0f)
	);

	cameraLayout = DescriptorSetLayoutBuilder(device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("CameraMatrices"); //Get our camera matrices...
	cameraDescriptor = CreateDescriptorSet(device , renderer->GetDescriptorPool(), *cameraLayout);

	WriteBufferDescriptor(device, *cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, cameraBuffer);

	nullLayout = DescriptorSetLayoutBuilder(device).Build("null layout");

	SetNullDescriptor(device, *nullLayout);
}

void VulkanTutorial::BuildCamera() {
	camera.SetFieldOfVision(45.0f)
		.SetNearPlane(0.1f)
		.SetFarPlane(1000.0f);
		
	cameraBuffer = BufferBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.WithHostVisibility()
		.WithPersistentMapping()
		.Build(sizeof(Matrix4) * 2, "Camera Buffer");

	camera.SetController(controller);

	controller.MapAxis(0, "Sidestep");
	controller.MapAxis(1, "UpDown");
	controller.MapAxis(2, "Forward");

	controller.MapAxis(3, "XLook");
	controller.MapAxis(4, "YLook");
}

void VulkanTutorial::UpdateCamera(float dt) {
	controller.Update(dt);
	camera.UpdateCamera(dt);
}

void VulkanTutorial::RunFrame(float dt) {
	Update(dt);

	renderer->BeginFrame();
	RenderFrame(dt);
	renderer->EndFrame();
	renderer->SwapBuffers();
};

void VulkanTutorial::UploadCameraUniform() {
	Matrix4* cameraMatrices = (Matrix4*)cameraBuffer.Data();
	cameraMatrices[0] = camera.BuildViewMatrix();
	cameraMatrices[1] = camera.BuildProjectionMatrix(hostWindow.GetScreenAspect());
}

UniqueVulkanMesh VulkanTutorial::GenerateTriangle() {
	VulkanMesh* triMesh = new VulkanMesh();
	triMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(0,1,0) });
	triMesh->SetVertexColours({ Vector4(1,0,0,1), Vector4(0,1,0,1), Vector4(0,0,1,1) });
	triMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(0.5, 1) });
	triMesh->SetVertexIndices({ 0,1,2 });

	triMesh->SetDebugName("Triangle");
	triMesh->SetPrimitiveType(NCL::GeometryPrimitive::Triangles);
	triMesh->UploadToGPU(renderer);

	return UniqueVulkanMesh(triMesh);
}

UniqueVulkanMesh VulkanTutorial::GenerateQuad() {
	VulkanMesh* quadMesh = new VulkanMesh();
	quadMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	quadMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	quadMesh->SetVertexIndices({ 0,1,3,2 });
	quadMesh->SetDebugName("Fullscreen Quad");
	quadMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);
	quadMesh->UploadToGPU(renderer);

	return UniqueVulkanMesh(quadMesh);
}

UniqueVulkanMesh VulkanTutorial::GenerateGrid() {
	VulkanMesh* gridMesh = new VulkanMesh();
	gridMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	gridMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	gridMesh->SetVertexIndices({ 0,1,3,2 });
	gridMesh->SetDebugName("Test Grid");
	gridMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);
	gridMesh->UploadToGPU(renderer);

	return UniqueVulkanMesh(gridMesh);
}

UniqueVulkanMesh VulkanTutorial::LoadMesh(const string& filename, vk::BufferUsageFlags flags) {
	VulkanMesh* newMesh = new VulkanMesh();

	MshLoader::LoadMesh(filename, *newMesh);

	newMesh->UploadToGPU(renderer, flags);

	return UniqueVulkanMesh(newMesh);
}

UniqueVulkanTexture VulkanTutorial::LoadTexture(const string& filename) {
	return TextureBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandBuffer::Graphics))
		.UsingQueue(renderer->GetQueue(CommandBuffer::Graphics))
		.BuildFromFile(filename);
}

UniqueVulkanTexture VulkanTutorial::LoadCubemap(
	const std::string& negativeXFile, const std::string& positiveXFile,
	const std::string& negativeYFile, const std::string& positiveYFile,
	const std::string& negativeZFile, const std::string& positiveZFile,
	const std::string& debugName) {

	return TextureBuilder(renderer->GetDevice(), renderer->GetMemoryAllocator())
		.UsingPool(renderer->GetCommandPool(CommandBuffer::Graphics))
		.UsingQueue(renderer->GetQueue(CommandBuffer::Graphics))
		.BuildCubemapFromFile(negativeXFile, positiveXFile,
			negativeYFile, positiveYFile,
			negativeZFile, positiveZFile,
			debugName
		);
}

void VulkanTutorial::RenderSingleObject(RenderObject& o, vk::CommandBuffer  toBuffer, VulkanPipeline& toPipeline, int descriptorSet) {
	toBuffer.pushConstants(*toPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Matrix4), (void*)&o.transform);
	toBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *toPipeline.layout, descriptorSet, 1, &*o.descriptorSet, 0, nullptr);
	
	if (o.mesh->GetSubMeshCount() == 0) {
		o.mesh->Draw(toBuffer);
	}
	else {
		for (unsigned int i = 0; i < o.mesh->GetSubMeshCount(); ++i) {
			o.mesh->DrawLayer(i, toBuffer);
		}
	}
}

VulkanInitialisation VulkanTutorial::DefaultInitialisation() {
	VulkanInitialisation vkInit;

	vkInit.depthStencilFormat = vk::Format::eD32SfloatS8Uint;

	vkInit.majorVersion = 1;
	vkInit.minorVersion = 3;

	vkInit.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back("VK_KHR_dynamic_rendering");		//Now in core 1.3
	vkInit.deviceExtensions.push_back("VK_KHR_maintenance4");			//Now in core 1.3
	vkInit.deviceExtensions.push_back("VK_KHR_depth_stencil_resolve");	//Now in core 1.2
	vkInit.deviceExtensions.push_back("VK_KHR_create_renderpass2");		//Now in core 1.2
	vkInit.deviceExtensions.push_back("VK_KHR_synchronization2");		//Now in core 1.2
	vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	vkInit.instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef WIN32
	vkInit.instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	vkInit.deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	vkInit.instanceLayers.push_back("VK_LAYER_KHRONOS_validation");

	static vk::PhysicalDeviceRobustness2FeaturesEXT robustness;
	robustness.nullDescriptor = true;

	static vk::PhysicalDeviceSynchronization2Features syncFeatures;
	syncFeatures.synchronization2 = true;

	static vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering;
	dynamicRendering.dynamicRendering = true;


	vkInit.features.push_back((void*)&robustness);
	vkInit.features.push_back((void*)&syncFeatures);
	vkInit.features.push_back((void*)&dynamicRendering);

//#ifdef USE_RAY_TRACING
//	vkInit.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
//	vkInit.deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
//	vkInit.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
//	vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
//	vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
//
//	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
//		.accelerationStructure = true
//	};
//
//	vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayFeatures = {
//		.rayTracingPipeline = true
//	};
//
//	vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
//		.bufferDeviceAddress = true
//	};
//
//	vkInit.features.push_back((void*)&accelFeatures);
//	vkInit.features.push_back((void*)&rayFeatures);
//	vkInit.features.push_back((void*)&deviceAddressfeature);
//#endif

	return vkInit;
}