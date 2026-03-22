/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanTutorial.h"
#include "VulkanVMAMemoryManager.h"
#include "MshLoader.h"
#include "GLTFLoader.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanTutorialEntry* VulkanTutorialEntry::s_listStartPtr = nullptr;

VulkanTutorial::VulkanTutorial(Window& window, VulkanInitialisation& vkInit) : m_hostWindow(window), m_controller(*window.GetKeyboard(), *window.GetMouse()) {
	m_runTime	= 0.0f;
	m_vkInit	= vkInit;

	GLTFLoader::SetMeshConstructionFunction(
		[&]()-> std::shared_ptr<Mesh> {return std::shared_ptr<Mesh> (new VulkanMesh()); }
	);

	GLTFLoader::SetTextureConstructionFunction(
		[&](std::string& input) ->  SharedTexture {return std::shared_ptr<Texture>(LoadTexture(input).release()); }
	);
}

VulkanTutorial::~VulkanTutorial() {
	m_renderer->GetDevice().waitIdle();
	m_memoryManager->DiscardBuffer(m_cameraBuffer, DiscardMode::Immediate);
	m_cameraDescriptor.reset();
	m_cameraLayout.reset();
	m_nullLayout.reset();
	m_defaultSampler.reset();

	m_triangleMesh.reset();
	m_quadMesh.reset();
	m_gridMesh.reset();
	m_cubeMesh.reset();
	m_sphereMesh.reset();

	delete m_memoryManager;
	delete m_renderer;
}

void VulkanTutorial::Finish() const {
	m_renderer->GetDevice().waitIdle();
}

void VulkanTutorial::Initialise() {
	m_renderer		= new VulkanRenderer(m_hostWindow, m_vkInit);
	m_memoryManager = new VulkanVMAMemoryManager(m_renderer->GetDevice(), m_renderer->GetPhysicalDevice(), m_renderer->GetVulkanInstance(), m_vkInit);
	BuildCamera();

	FrameContext const& context = m_renderer->GetFrameContext();

	vk::Device device = context.device;

	m_defaultSampler = device.createSamplerUnique(
		vk::SamplerCreateInfo()
		.setAnisotropyEnable(false)
		.setMaxAnisotropy(16)
		.setMinFilter(vk::Filter::eLinear)
		.setMagFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMaxLod(80.0f)
	);

	m_cameraLayout = DescriptorSetLayoutBuilder(device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("CameraMatrices"); //Get our m_camera matrices...
	m_cameraDescriptor = CreateDescriptorSet(device, context.descriptorPool, *m_cameraLayout);

	WriteBufferDescriptor(device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);

	m_nullLayout = DescriptorSetLayoutBuilder(device).Build("null layout");
	SetNullDescriptor(device, *m_nullLayout);


	m_triangleMesh	= GenerateTriangle();
	m_quadMesh		= GenerateQuad();
	m_gridMesh		= GenerateGrid();
	m_cubeMesh		= LoadMesh("Cube.msh");
	m_sphereMesh	= LoadMesh("Sphere.msh");
}

void VulkanTutorial::BuildCamera() {
	m_camera.SetFieldOfVision(45.0f)
		.SetNearPlane(0.1f)
		.SetFarPlane(1000.0f);
	
	m_cameraBuffer = m_memoryManager->CreateBuffer(
		{
			.size	= sizeof(Matrix4) * 2,
			.usage	= vk::BufferUsageFlagBits::eUniformBuffer,
		},
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent,
		"Camera Buffer"
	);

	m_camera.SetController(m_controller);

	m_controller.MapAxis(0, "Sidestep");
	m_controller.MapAxis(1, "UpDown");
	m_controller.MapAxis(2, "Forward");

	m_controller.MapAxis(3, "XLook");
	m_controller.MapAxis(4, "YLook");
}

void VulkanTutorial::UpdateCamera(float dt) {
	m_controller.Update(dt);
	m_camera.UpdateCamera(dt);
}

void VulkanTutorial::RunFrame(float dt) {
	if (m_hostWindow.IsMinimised()) {
		return;
	}	
	m_renderer->BeginFrame();

	Update(dt);

	m_memoryManager->Update();

	UploadCameraUniform();
	RenderFrame(dt);
	m_renderer->EndFrame();
	m_renderer->SwapBuffers();
};

void VulkanTutorial::WindowEventHandler(WindowEvent e, uint32_t w, uint32_t h) {
	if (e == WindowEvent::Resize || e == WindowEvent::Maximize) {
		m_renderer->OnWindowResize(w, h);
		OnWindowResize(w, h);
	}
}

void VulkanTutorial::UploadCameraUniform() {
	Matrix4* cameraMatrices = m_cameraBuffer.Map<Matrix4>();
	cameraMatrices[0] = m_camera.BuildViewMatrix();
	cameraMatrices[1] = m_camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect());
	m_cameraBuffer.Unmap();
}

UniqueVulkanMesh VulkanTutorial::GenerateTriangle() {
	VulkanMesh* triMesh = new VulkanMesh();
	triMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(0,1,0) });
	triMesh->SetVertexColours({ Vector4(1,0,0,1), Vector4(0,1,0,1), Vector4(0,0,1,1) });
	triMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(0.5, 1) });
	triMesh->SetVertexIndices({ 0,1,2 });

	triMesh->SetDebugName("Triangle");
	triMesh->SetPrimitiveType(NCL::GeometryPrimitive::Triangles);

	UploadMeshWait(*triMesh);

	return UniqueVulkanMesh(triMesh);
}

UniqueVulkanMesh VulkanTutorial::GenerateQuad() {
	VulkanMesh* quadMesh = new VulkanMesh();
	quadMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	quadMesh->SetVertexTextureCoords({ Vector2(0,1), Vector2(1,1), Vector2(1, 0), Vector2(0, 0) });
	quadMesh->SetVertexIndices({ 0,1,3,2 });
	quadMesh->SetDebugName("Fullscreen Quad");
	quadMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);

	UploadMeshWait(*quadMesh);

	return UniqueVulkanMesh(quadMesh);
}

UniqueVulkanMesh VulkanTutorial::GenerateGrid() {
	VulkanMesh* gridMesh = new VulkanMesh();
	gridMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	gridMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	gridMesh->SetVertexIndices({ 0,1,3,2 });
	gridMesh->SetDebugName("Test Grid");
	gridMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);

	UploadMeshWait(*gridMesh);

	return UniqueVulkanMesh(gridMesh);
}

UniqueVulkanMesh VulkanTutorial::LoadMesh(const string& filename, vk::BufferUsageFlags flags) {
	VulkanMesh* newMesh = new VulkanMesh();

	MshLoader::LoadMesh(filename, *newMesh);
	UploadMeshWait(*newMesh, flags);
	return UniqueVulkanMesh(newMesh);
}

void VulkanTutorial::UploadMeshWait(VulkanMesh& m, vk::BufferUsageFlags flags) {
	FrameContext const& context = m_renderer->GetFrameContext();

	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanMesh upload");

	m.UploadToGPU(*cmdBuffer, m_memoryManager, flags);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);
}

UniqueVulkanTexture VulkanTutorial::LoadTexture(const string& filename) {
	FrameContext const& context = m_renderer->GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanTexture upload");
	
	UniqueVulkanTexture tex = TextureBuilder(context.device, *m_memoryManager)
	.WithCommandBuffer(*cmdBuffer)
	.BuildFromFile(filename);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	return tex;
}

UniqueVulkanTexture VulkanTutorial::LoadCubemap(
	const std::string& negativeXFile, const std::string& positiveXFile,
	const std::string& negativeYFile, const std::string& positiveYFile,
	const std::string& negativeZFile, const std::string& positiveZFile,
	const std::string& debugName) {

	FrameContext const& context = m_renderer->GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanTexture upload");

	UniqueVulkanTexture tex = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(*cmdBuffer)
		.BuildCubemapFromFile(negativeXFile, positiveXFile,
			negativeYFile, positiveYFile,
			negativeZFile, positiveZFile,
			debugName
	);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);
	return tex;
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
	VulkanInitialisation m_vkInit;

	m_vkInit.depthStencilFormat = vk::Format::eD32SfloatS8Uint;

	m_vkInit.majorVersion = 1;
	m_vkInit.minorVersion = 3;

	m_vkInit.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//vkInit.deviceExtensions.push_back("VK_KHR_dynamic_rendering");		//Now in core 1.3
	//vkInit.deviceExtensions.push_back("VK_KHR_maintenance4");			//Now in core 1.3
	//vkInit.deviceExtensions.push_back("VK_KHR_depth_stencil_resolve");	//Now in core 1.2
	//vkInit.deviceExtensions.push_back("VK_KHR_create_renderpass2");		//Now in core 1.2
	//vkInit.deviceExtensions.push_back("VK_KHR_synchronization2");		//Now in core 1.2
	m_vkInit.deviceExtensions.push_back("VK_EXT_robustness2");
	m_vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	m_vkInit.instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	m_vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	m_vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef WIN32
	m_vkInit.instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	m_vkInit.deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	m_vkInit.instanceLayers.push_back("VK_LAYER_KHRONOS_validation");

	static vk::PhysicalDeviceRobustness2FeaturesEXT robustness{
		.nullDescriptor = true
	};
	
	static vk::PhysicalDeviceSynchronization2Features syncFeatures{
		.synchronization2 = true
	};

	static vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{
		.dynamicRendering = true
	};

	static vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphores{
		.timelineSemaphore = true
	};

	static vk::PhysicalDeviceScalarBlockLayoutFeatures scalarFeatures{
		.scalarBlockLayout = true
	};
	
	m_vkInit.features.push_back((void*)&robustness);
	m_vkInit.features.push_back((void*)&syncFeatures);
	m_vkInit.features.push_back((void*)&dynamicRendering);
	m_vkInit.features.push_back((void*)&timelineSemaphores);
	m_vkInit.features.push_back((void*)&scalarFeatures);

	m_vkInit.framesInFlight = 1;

	return m_vkInit;
}

VulkanTutorial* VulkanTutorial::CreateTutorial(const std::string& name, VulkanInitialisation& vkInit) {
	VulkanTutorialEntry* e = VulkanTutorialEntry::s_listStartPtr;

	while (e) {
		if (e->m_name == name) {
			std::cout << "Running tutorial " << e->m_name << "\n";
			return e->m_creatorFunc(*NCL::Window::GetWindow(), vkInit);
		}
		e = e->m_nodeChain;
	}
	return nullptr;
}

VulkanTutorial* VulkanTutorial::CreateTutorial(int& chainID, VulkanInitialisation& vkInit) {
	VulkanTutorialEntry* e = VulkanTutorialEntry::s_listStartPtr;
	int index = 0;
	while (e && index != chainID) {
		e = e->m_nodeChain;
		index++;
	}
	chainID++;
	if (e) {
		std::cout << "Running tutorial " << e->m_name << "\n";
		return e->m_creatorFunc(*NCL::Window::GetWindow(), vkInit);
	}

	return nullptr;
}