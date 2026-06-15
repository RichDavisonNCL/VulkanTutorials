/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Vulkanrenderer.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"
#include "VulkanTextureBuilder.h"
#include "VulkanDescriptorSetLayoutBuilder.h"

#include "VulkanUtils.h"

#ifdef _WIN32
#include "Win32Window.h"
using namespace NCL::Win32Code;
#endif

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanRenderer::VulkanRenderer(Window& window, const VulkanInitialisation& vkInitInfo) : RendererBase(window)
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = Vulkan::dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	m_vkInit = vkInitInfo;

	for (uint32_t i = 0; i < CommandType::MAX_COMMAND_TYPES; ++i) {
		m_queueFamilies[i] = -1;
	}

	InitInstance();

	InitPhysicalDevice();

	InitGPUDevice();

	InitCommandPools();
	InitDefaultDescriptorPool();

	OnWindowResize(window.GetScreenSize().x, window.GetScreenSize().y);
	InitFrameStates(m_vkInit.framesInFlight);

	m_pipelineCache = m_device.createPipelineCache(vk::PipelineCacheCreateInfo());

	const FrameContext& context = GetFrameContext();

	context.cmdBuffer.begin(vk::CommandBufferBeginInfo());


	vk::SemaphoreTypeCreateInfo typeCreateInfo{
		.semaphoreType = vk::SemaphoreType::eTimeline,
		.initialValue = 0
	};
	vk::SemaphoreCreateInfo createInfo{
		.pNext = &typeCreateInfo,
	};
	m_flightSempaphore = m_device.createSemaphore(createInfo);
}

VulkanRenderer::~VulkanRenderer() {
	m_device.waitIdle();

	for(ChainState & c : m_swapStates) {
		m_device.destroyFramebuffer(c.frameBuffer);
		m_device.destroySemaphore(c.presentSempaphore);
		m_device.destroyFence(c.presentFence);
		m_device.destroyImageView(c.colourView);
	}

	for (AcquireState a : m_acquireStates) {
		m_device.destroySemaphore(a.acquireSempaphore);
		m_device.destroyFence(a.acquireFence);
	}

	m_frameContexts.clear();
	m_device.destroyDescriptorPool(m_defaultDescriptorPool);
	m_device.destroySwapchainKHR(m_swapChain);

	m_device.destroySemaphore(m_flightSempaphore);

	for (auto& c : m_commandPools) {
		if (c) {
			m_device.destroyCommandPool(c);
		}
	}

	m_device.destroyImageView(m_depthView);
	m_device.destroyImage(m_depthImage);
	m_device.freeMemory(m_depthMemory);
	
	m_device.destroyRenderPass(m_defaultRenderPass);
	m_device.destroyPipelineCache(m_pipelineCache);
	m_device.destroy(); //Destroy everything except instance before this gets destroyed!

	m_instance.destroySurfaceKHR(m_surface);
	m_instance.destroy();
}

bool VulkanRenderer::InitInstance() {
	vk::ApplicationInfo appInfo = {
		.pApplicationName	= hostWindow.GetTitle().c_str(),
		.apiVersion			= VK_MAKE_VERSION(m_vkInit.majorVersion, m_vkInit.minorVersion, 0)
	};
		
	vk::InstanceCreateInfo instanceInfo = {
		.flags = {},
		.pApplicationInfo		 = &appInfo,
		.enabledLayerCount		 = (uint32_t)m_vkInit.instanceLayers.size(),
		.ppEnabledLayerNames	 = m_vkInit.instanceLayers.data(),
		.enabledExtensionCount	 = (uint32_t)m_vkInit.instanceExtensions.size(),
		.ppEnabledExtensionNames = m_vkInit.instanceExtensions.data()
	};
		
	m_instance = vk::createInstance(instanceInfo);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

	return true;
}

bool	VulkanRenderer::InitPhysicalDevice() {
	auto enumResult = m_instance.enumeratePhysicalDevices();

	if (enumResult.empty()) {
		return false; //Guess there's no Vulkan capable devices?!
	}

	m_physicalDevice = enumResult[0];
	for (auto& i : enumResult) {
		if (i.getProperties().deviceType == m_vkInit.idealGPU) {
			m_physicalDevice = i;
			break;
		}
	}

	std::cout << __FUNCTION__ << " Vulkan using physical device " << m_physicalDevice.getProperties().deviceName << "\n";

	return true;
}

bool VulkanRenderer::InitGPUDevice() {
	InitSurface();
	InitDeviceQueueIndices();

	float queuePriority = 0.0f;

	std::vector< vk::DeviceQueueCreateInfo> queueInfos;

	queueInfos.emplace_back(vk::DeviceQueueCreateInfo()
		.setQueueCount(1)
		.setQueueFamilyIndex(m_queueFamilies[CommandType::Type::Graphics])
		.setPQueuePriorities(&queuePriority)
	);

	if (m_queueFamilies[CommandType::Type::AsyncCompute] != m_queueFamilies[CommandType::Type::Graphics]){
		queueInfos.emplace_back(vk::DeviceQueueCreateInfo()
			.setQueueCount(1)
			.setQueueFamilyIndex(m_queueFamilies[CommandType::Type::AsyncCompute])
			.setPQueuePriorities(&queuePriority)
		);
	}
	if (m_queueFamilies[CommandType::Type::Copy] != m_queueFamilies[CommandType::Type::Graphics]) {
		queueInfos.emplace_back(vk::DeviceQueueCreateInfo()
			.setQueueCount(1)
			.setQueueFamilyIndex(m_queueFamilies[CommandType::Copy])
			.setPQueuePriorities(&queuePriority)
		);
	}
	if (m_queueFamilies[CommandType::Type::Present] != m_queueFamilies[CommandType::Type::Graphics]) {
		queueInfos.emplace_back(vk::DeviceQueueCreateInfo()
			.setQueueCount(1)
			.setQueueFamilyIndex(m_queueFamilies[CommandType::Present])
			.setPQueuePriorities(&queuePriority)
		);
	}

	vk::PhysicalDeviceFeatures2 deviceFeatures;
	m_physicalDevice.getFeatures2(&deviceFeatures);

	if (!m_vkInit.features.empty()) {
		for (int i = 1; i < m_vkInit.features.size(); ++i) {
			vk::PhysicalDeviceFeatures2* prevStruct = (vk::PhysicalDeviceFeatures2*)m_vkInit.features[i-1];
			prevStruct->pNext = m_vkInit.features[i];
		}
		deviceFeatures.pNext = m_vkInit.features[0];
	}

	vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(queueInfos.size())
		.setPQueueCreateInfos(queueInfos.data());
	
	createInfo.setEnabledLayerCount((uint32_t)m_vkInit.deviceLayers.size())
		.setPpEnabledLayerNames(m_vkInit.deviceLayers.data())
		.setEnabledExtensionCount((uint32_t)m_vkInit.deviceExtensions.size())
		.setPpEnabledExtensionNames(m_vkInit.deviceExtensions.data());

	createInfo.pNext = &deviceFeatures;

	m_device = m_physicalDevice.createDevice(createInfo);
	Vulkan::SetDebugName(m_device, m_device, "GPU Device");
	Vulkan::SetDebugName(m_device, m_physicalDevice, "Physical Device");

	m_queues[CommandType::Graphics]		= m_device.getQueue(m_queueFamilies[CommandType::Type::Graphics], 0);
	m_queues[CommandType::AsyncCompute]	= m_device.getQueue(m_queueFamilies[CommandType::Type::AsyncCompute], 0);
	m_queues[CommandType::Copy]			= m_device.getQueue(m_queueFamilies[CommandType::Type::Copy], 0);
	m_queues[CommandType::Present]		= m_device.getQueue(m_queueFamilies[CommandType::Type::Present], 0);

	Vulkan::SetDebugName(m_device, m_queues[CommandType::Graphics], "Graphics Queue");

	if (m_queues[CommandType::AsyncCompute] != m_queues[CommandType::Graphics]) {
		Vulkan::SetDebugName(m_device, m_queues[CommandType::AsyncCompute], "Compute Queue");
	}
	if (m_queues[CommandType::Copy] != m_queues[CommandType::Graphics]) {
		Vulkan::SetDebugName(m_device, m_queues[CommandType::Copy], "Copy Queue");
	}
	if (m_queues[CommandType::Present] != m_queues[CommandType::Graphics]) {
		Vulkan::SetDebugName(m_device, m_queues[CommandType::Present], "Present Queue");
	}

	m_deviceMemoryProperties	= m_physicalDevice.getMemoryProperties();
	m_deviceProperties			= m_physicalDevice.getProperties();

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

	return true;
}

bool VulkanRenderer::InitSurface() {
#ifdef _WIN32
	Win32Window* window = (Win32Window*)&hostWindow;

	m_surface = m_instance.createWin32SurfaceKHR(
		{
			.flags = {},
			.hinstance = window->GetInstance(),
			.hwnd = window->GetHandle()
		}
	);
#endif

	auto formats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);
	if (formats.empty()) {
		return false;
	}
	m_surfaceFormat	= formats[0].format;
	m_surfaceSpace	= formats[0].colorSpace;

	if (m_vkInit.useHDRSurface) {
		for (auto& i : formats) {
			if ((uint64_t)i.format >= VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
				(uint64_t)i.format <= VK_FORMAT_A2B10G10R10_SINT_PACK32)
			{
				m_surfaceFormat = i.format;
				m_surfaceSpace = i.colorSpace;
				break;
			}
		}

	}
	return true;
}

void	VulkanRenderer::InitFrameStates(uint32_t m_framesInFlight) {
	m_frameContexts.resize(m_framesInFlight);
	m_acquireStates.resize(m_framesInFlight);
	auto buffers = m_device.allocateCommandBuffers(
		{
			.commandPool = m_commandPools[CommandType::Graphics],
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = m_framesInFlight
		}
	);

	uint32_t index = 0;
	for (FrameContext& context : m_frameContexts) {
		context.device				= m_device;
		context.descriptorPool		= m_defaultDescriptorPool;

		context.cmdBuffer			= buffers[index];

		Vulkan::SetDebugName(m_device, context.cmdBuffer, "Context Cmds " + std::to_string(index));

		context.viewport			= m_defaultViewport;
		context.screenRect			= m_defaultScreenRect;

		context.colourFormat		= m_surfaceFormat;

		context.depthImage			= m_depthImage;
		context.depthView			= m_depthView;;
		context.depthFormat			= m_vkInit.depthStencilFormat;



		for (int i = 0; i < CommandType::Type::MAX_COMMAND_TYPES; ++i) {
			context.commandPools[i]		= m_commandPools[i];
			context.queues[i]			= m_queues[i];
			context.queueFamilies[i]	= m_queueFamilies[i];
		}
		index++;
	}
	for(AcquireState& s : m_acquireStates) {
		s.acquireSempaphore	= m_device.createSemaphore({});
		s.acquireFence		= m_device.createFence({});
	}
}

void VulkanRenderer::InitBufferChain(vk::CommandBuffer  cmdBuffer) {
	vk::SwapchainKHR oldChain					= m_swapChain;

	vk::SurfaceCapabilitiesKHR surfaceCaps = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

	vk::Extent2D swapExtents = vk::Extent2D((int)hostWindow.GetScreenSize().x, (int)hostWindow.GetScreenSize().y);

	auto presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_surface); //Type is of vector of PresentModeKHR

	vk::PresentModeKHR currentPresentMode	= vk::PresentModeKHR::eFifo;

	for (const auto& i : presentModes) {
		if (i == m_vkInit.idealPresentMode) {
			currentPresentMode = i;
			break;
		}
	}

	if (currentPresentMode != m_vkInit.idealPresentMode) {
		std::cout << __FUNCTION__ << " Vulkan cannot use chosen present mode! Defaulting to FIFO...\n";
	}

	vk::SurfaceTransformFlagBitsKHR idealTransform;

	if (surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
		idealTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	}
	else {
		idealTransform = surfaceCaps.currentTransform;
	}

	int idealImageCount = surfaceCaps.minImageCount + 1;
	if (surfaceCaps.maxImageCount > 0) {
		idealImageCount = std::min(idealImageCount, (int)surfaceCaps.maxImageCount);
	}

	vk::SwapchainCreateInfoKHR swapInfo;

	swapInfo.setPresentMode(currentPresentMode)
		.setPreTransform(idealTransform)
		.setSurface(m_surface)
		.setImageColorSpace(m_surfaceSpace)
		.setImageFormat(m_surfaceFormat)
		.setImageExtent(swapExtents)
		.setMinImageCount(idealImageCount)
		.setOldSwapchain(oldChain)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	m_swapChain = m_device.createSwapchainKHR(swapInfo);

	if (!m_swapStates.empty()) {
		for(auto& i : m_swapStates) {
			m_device.destroyImageView(i.colourView);
			m_device.destroyFramebuffer(i.frameBuffer);
			m_device.destroySemaphore(i.presentSempaphore);
			m_device.destroyFence(i.presentFence);
		}
	}
	if (oldChain) {
		m_device.destroySwapchainKHR(oldChain);
	}

	auto images = m_device.getSwapchainImagesKHR(m_swapChain);

	m_swapStates.resize(images.size() + 1);

	for(int i = 0; i < m_swapStates.size(); ++i) {
		ChainState& chain = m_swapStates[i];
		chain.presentSempaphore = m_device.createSemaphore({});
		chain.presentFence		= m_device.createFence({});
		if (i < images.size()) {
			chain.colourImage = images[i];

			chain.colourView = m_device.createImageView(
				vk::ImageViewCreateInfo()
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
				.setFormat(m_surfaceFormat)
				.setImage(chain.colourImage)
				.setViewType(vk::ImageViewType::e2D)
			);

			vk::ImageView attachments[2]{
				chain.colourView ,
				m_depthView
			};

			vk::FramebufferCreateInfo createInfo = vk::FramebufferCreateInfo()
				.setWidth(hostWindow.GetScreenSize().x)
				.setHeight(hostWindow.GetScreenSize().y)
				.setLayers(1)
				.setAttachmentCount(std::size(attachments))
				.setPAttachments(attachments)
				.setRenderPass(m_defaultRenderPass);

			chain.frameBuffer = m_device.createFramebuffer(createInfo);
		}
	}
	m_currentSwap		= 0; //??
	m_currentSwapState	= &m_swapStates.back();
}

void	VulkanRenderer::InitCommandPools() {	
	for (uint32_t i = 0; i < CommandType::MAX_COMMAND_TYPES; ++i) {
		m_commandPools[i] = m_device.createCommandPool(
			{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = m_queueFamilies[i]
			}
		);
	}
	Vulkan::SetDebugName(m_device, m_commandPools[CommandType::Graphics], "Graphics Command Pool");
	Vulkan::SetDebugName(m_device, m_commandPools[CommandType::AsyncCompute], "Async Command Pool");
	Vulkan::SetDebugName(m_device, m_commandPools[CommandType::Copy], "Copy Command Pool");
	Vulkan::SetDebugName(m_device, m_commandPools[CommandType::Present], "Present Command Pool");
}

bool VulkanRenderer::InitDeviceQueueIndices() {
	std::vector<vk::QueueFamilyProperties> deviceQueueProps = m_physicalDevice.getQueueFamilyProperties();

	VkBool32 supportsPresent = false;

	int gfxBits		= INT_MAX;
	int computeBits = INT_MAX;
	int copyBits	= INT_MAX;

	for (unsigned int i = 0; i < deviceQueueProps.size(); ++i) {
		supportsPresent = m_physicalDevice.getSurfaceSupportKHR(i, m_surface);

		int queueBitCount = std::popcount((uint32_t)deviceQueueProps[i].queueFlags);

		if (deviceQueueProps[i].queueFlags & vk::QueueFlagBits::eGraphics && queueBitCount < gfxBits) {
			m_queueFamilies[CommandType::Graphics] = i;
			gfxBits = queueBitCount;
			if (supportsPresent && m_queueFamilies[CommandType::Present] == -1) {
				m_queueFamilies[CommandType::Present] = i;
			}
		}

		if (deviceQueueProps[i].queueFlags & vk::QueueFlagBits::eCompute && queueBitCount < computeBits) {
			m_queueFamilies[CommandType::AsyncCompute] = i;
			computeBits = queueBitCount;
		}

		if (deviceQueueProps[i].queueFlags & vk::QueueFlagBits::eTransfer && queueBitCount < copyBits) {
			m_queueFamilies[CommandType::Copy] = i;
			copyBits = queueBitCount;
		}
	}

	if (m_queueFamilies[CommandType::Graphics] == -1) {
		return false;
	}

	if (m_queueFamilies[CommandType::AsyncCompute] == -1) {
		m_queueFamilies[CommandType::AsyncCompute] = m_queueFamilies[CommandType::Graphics];
	}
	else if(m_queueFamilies[CommandType::AsyncCompute] != m_queueFamilies[CommandType::Graphics]) {
		std::cout << __FUNCTION__ << " Device supports async compute!\n";
	}
	else {
		std::cout << __FUNCTION__ << " Device does NOT support async compute!\n";
	}

	if (m_queueFamilies[CommandType::Copy] == -1) {
		m_queueFamilies[CommandType::Copy] = m_queueFamilies[CommandType::Copy];
	}
	else if(m_queueFamilies[CommandType::Copy] != m_queueFamilies[CommandType::Graphics]){
		std::cout << __FUNCTION__ << " Device supports async copy!\n";
	}
	else {
		std::cout << __FUNCTION__ << " Device does NOT support async copy!\n";
	}

	return true;
}

bool VulkanRenderer::SupportsAsyncCompute() const {
	return m_queueFamilies[CommandType::AsyncCompute] != m_queueFamilies[CommandType::Graphics];
}

bool VulkanRenderer::SupportsAsyncCopy()	const {
	return m_queueFamilies[CommandType::Copy] != m_queueFamilies[CommandType::Graphics];
}

bool VulkanRenderer::SupportsAsyncPresent() const {
	return m_queueFamilies[CommandType::Present] != m_queueFamilies[CommandType::Graphics];
}

void VulkanRenderer::OnWindowResize(int width, int height) {
	if (width == windowSize.x && height == windowSize.y) {
		m_device.waitIdle();
		return;
	}
	if (width == 0 && height == 0) {
		return;
	}
	windowSize = { width, height };

	m_defaultScreenRect = {
		.offset = {0,0},
		.extent = { (uint32_t)windowSize.x, (uint32_t)windowSize.y }
	};

	m_defaultScissor = {
		.offset = {0,0},
		.extent = { (uint32_t)windowSize.x, (uint32_t)windowSize.y }
	};
		
	if (m_vkInit.useOpenGLCoordinates) {
		m_defaultViewport = vk::Viewport(0.0f, (float)windowSize.y, (float)windowSize.x, (float)windowSize.y, -1.0f, 1.0f);
	}
	else {
		m_defaultViewport = vk::Viewport(0.0f, (float)windowSize.y, (float)windowSize.x, -(float)windowSize.y, 0.0f, 1.0f);
	}

	std::cout << __FUNCTION__ << " New dimensions: " << windowSize.x << " , " << windowSize.y << "\n";

	m_device.waitIdle();
	
	CreateDepthBufer(hostWindow.GetScreenSize().x, hostWindow.GetScreenSize().y);
	
	InitDefaultRenderPass();

	vk::UniqueCommandBuffer cmds = CmdBufferCreateBegin(m_device, m_commandPools[CommandType::Graphics], "Window resize cmds");
	InitBufferChain(*cmds);

	m_device.waitIdle();

	CompleteResize();

	CmdBufferEndSubmitWait(*cmds, m_device, m_queues[CommandType::Graphics]);

	m_device.waitIdle();
}

void VulkanRenderer::CompleteResize() {

}

void	VulkanRenderer::BeginFrame() {
	const FrameContext& context = GetFrameContext();

	//Clear out any commands from the constructor
	if (m_globalFrameID == 0) {
		CmdBufferEndSubmitWait(context.cmdBuffer, m_device, m_queues[CommandType::Graphics]);
	}

	//First we need to prevent the m_renderer from going too far ahead of the frames in flight max
	m_currentFrameContextID = (m_currentFrameContextID + 1) % m_frameContexts.size();

	m_currentFrameContext = &m_frameContexts[m_currentFrameContextID];
	m_currentAcquireState = &m_acquireStates[m_currentFrameContextID];

	////block on the waiting frame's timeline semaphore
	vk::SemaphoreWaitInfo waitInfo = {
		.semaphoreCount = 1,
		.pSemaphores	= &m_flightSempaphore,
		.pValues		= &(m_currentFrameContext->waitID),
	};
	vk::Result waitResult = m_device.waitSemaphores(waitInfo, UINT64_MAX);
	

	//Acquire our next swap image
	m_device.resetFences(m_currentAcquireState->acquireFence);
	uint32_t oldSwapIndex = m_currentSwap;
	m_currentSwap = m_device.acquireNextImageKHR(m_swapChain, UINT64_MAX,
		m_currentAcquireState->acquireSempaphore,
		m_currentAcquireState->acquireFence).value;

	m_currentSwapState = &(m_swapStates[m_currentSwap]);

	//Now we know the swap image, we can fill out our current frame state...
	m_currentFrameContext->frameID			= m_globalFrameID;
	m_currentFrameContext->cycleID			= m_currentFrameContextID;

	m_currentFrameContext->viewport			= m_defaultViewport;
	m_currentFrameContext->screenRect		= m_defaultScreenRect;

	m_currentFrameContext->colourImage		= m_currentSwapState->colourImage;
	m_currentFrameContext->colourView		= m_currentSwapState->colourView;
	m_currentFrameContext->colourFormat		= m_surfaceFormat;
	m_currentFrameContext->depthFormat		= m_vkInit.depthStencilFormat;

	context.cmdBuffer.reset({});

	context.cmdBuffer.begin(vk::CommandBufferBeginInfo());
	context.cmdBuffer.setViewport(0, 1, &m_defaultViewport);
	context.cmdBuffer.setScissor(0, 1, &m_defaultScissor);

	if (m_vkInit.autoBeginDynamicRendering) {
		BeginRenderToScreen(context.cmdBuffer);
	}
}

void VulkanRenderer::RenderFrame() {

}

void	VulkanRenderer::EndFrame() {
	const FrameContext& context = GetFrameContext();

	if (m_vkInit.autoBeginDynamicRendering) {
		context.cmdBuffer.endRendering();
	}
	TransitionColourToPresent(context.cmdBuffer, m_currentFrameContext->colourImage);
	if (hostWindow.IsMinimised()) {
		CmdBufferEndSubmitWait(context.cmdBuffer, m_device, m_queues[CommandType::Graphics]);
	}
	else {
		CmdBufferEndSubmit(context.cmdBuffer, m_queues[CommandType::Graphics]);
	}

	const uint64_t nextWaitID = m_globalFrameID + m_frameContexts.size();
	
	uint64_t signalValue[2] = {
		nextWaitID,
		nextWaitID, //Dummy timeline semaphore
	};

	vk::Semaphore signalSempaphores[2] = {
		m_flightSempaphore,
		m_currentSwapState->presentSempaphore
	};

	vk::TimelineSemaphoreSubmitInfo tlSubmit;
	tlSubmit.signalSemaphoreValueCount	= std::size(signalValue);
	tlSubmit.pSignalSemaphoreValues		= signalValue;

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo queueSubmit = {
		.pNext					= &tlSubmit,

		.waitSemaphoreCount		= 1,
		.pWaitSemaphores		= &m_currentAcquireState->acquireSempaphore,
		.pWaitDstStageMask		= &waitStage,

		.signalSemaphoreCount	= std::size(signalSempaphores),
		.pSignalSemaphores		= signalSempaphores,
	};

	m_queues[CommandType::Graphics].submit(queueSubmit);

	m_currentFrameContext->waitID = nextWaitID;

	m_globalFrameID++;
}

void VulkanRenderer::SwapBuffers() {
	if (!hostWindow.IsMinimised()) {
		vk::Queue	gfxQueue		= m_queues[CommandType::Graphics];
		vk::Result	presentResult = gfxQueue.presentKHR(
			{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores	= &m_currentSwapState->presentSempaphore,
				.swapchainCount		= 1,
				.pSwapchains		= &m_swapChain,
				.pImageIndices		= &m_currentSwap,
			}
		);	
	}
}

void	VulkanRenderer::InitDefaultRenderPass() {
	if (m_defaultRenderPass) {
		m_device.destroyRenderPass(m_defaultRenderPass);
	}
	vk::AttachmentDescription attachments[] = {
		vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFormat(m_surfaceFormat)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	,
		vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFormat(m_vkInit.depthStencilFormat)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	};

	vk::AttachmentReference references[] = {
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
	};

	vk::SubpassDescription m_subPass = vk::SubpassDescription()
		.setColorAttachmentCount(1)
		.setPColorAttachments(&references[0])
		.setPDepthStencilAttachment(&references[1])
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(2)
		.setPAttachments(attachments)
		.setSubpassCount(1)
		.setPSubpasses(&m_subPass);

	m_defaultRenderPass = m_device.createRenderPass(renderPassInfo);
}

void	VulkanRenderer::InitDefaultDescriptorPool() {
	std::vector< vk::DescriptorPoolSize> poolSizes;

	if (m_vkInit.defaultDescriptorPoolBufferCount > 0) {
		poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, m_vkInit.defaultDescriptorPoolBufferCount);
		poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, m_vkInit.defaultDescriptorPoolBufferCount);
		poolSizes.emplace_back(vk::DescriptorType::eUniformBufferDynamic, m_vkInit.defaultDescriptorPoolBufferCount);
		poolSizes.emplace_back(vk::DescriptorType::eStorageBufferDynamic, m_vkInit.defaultDescriptorPoolBufferCount);
	}
	if (m_vkInit.defaultDescriptorPoolImageCount > 0) {
		poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, m_vkInit.defaultDescriptorPoolImageCount);
		poolSizes.emplace_back(vk::DescriptorType::eSampledImage, m_vkInit.defaultDescriptorPoolImageCount);
		poolSizes.emplace_back(vk::DescriptorType::eStorageImage, m_vkInit.defaultDescriptorPoolImageCount);
	}
	if (m_vkInit.defaultDescriptorPoolSamplerCount > 0) {
		poolSizes.emplace_back(vk::DescriptorType::eSampler, m_vkInit.defaultDescriptorPoolSamplerCount);
	}
	if (m_vkInit.defaultDescriptorPoolAccelerationStructureCount > 0) {
		poolSizes.emplace_back(vk::DescriptorType::eAccelerationStructureKHR, m_vkInit.defaultDescriptorPoolAccelerationStructureCount);
	}
	
	uint32_t maxSets = 0;
	maxSets += m_vkInit.defaultDescriptorPoolBufferCount * 4;
	maxSets += m_vkInit.defaultDescriptorPoolImageCount * 3;
	maxSets += m_vkInit.defaultDescriptorPoolSamplerCount;
	maxSets += m_vkInit.defaultDescriptorPoolAccelerationStructureCount;

	vk::DescriptorPoolCreateInfo poolCreate = {
		.flags			= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | m_vkInit.defaultDescriptorPoolFlags,
		.maxSets		= maxSets,
		.poolSizeCount	= uint32_t(poolSizes.size()),
		.pPoolSizes		= poolSizes.data(),
	};

	m_defaultDescriptorPool = m_device.createDescriptorPool(poolCreate);
}

void	VulkanRenderer::BeginDefaultRenderPass(vk::CommandBuffer  cmds) {
	cmds.beginRenderPass(m_defaultBeginInfo, vk::SubpassContents::eInline);
}

void	VulkanRenderer::BeginRenderToScreen(vk::CommandBuffer  cmds) {
	vk::Result waitResult = m_device.waitForFences(m_currentAcquireState->acquireFence, true, ~0);
	TransitionUndefinedToColour(cmds, m_currentFrameContext->colourImage);

	cmds.setViewport(0, 1, &m_defaultViewport);
	cmds.setScissor(0, 1, &m_defaultScissor);

	vk::RenderingInfoKHR renderInfo;

	vk::RenderingAttachmentInfoKHR colourAttachment;
	colourAttachment.setImageView(m_currentFrameContext->colourView)
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f));

	vk::RenderingAttachmentInfoKHR depthAttachment;
	depthAttachment.setImageView(m_depthView)
		.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.clearValue.setDepthStencil({ 1.0f, ~0U });

	renderInfo.setColorAttachments(colourAttachment)
		.setPDepthAttachment(&depthAttachment)
		.setLayerCount(1)
		.setRenderArea(m_defaultScreenRect);

	cmds.beginRendering(renderInfo);
	cmds.setViewport(0, 1, &m_defaultViewport);
	cmds.setScissor( 0, 1, &m_defaultScissor);
}

VkBool32 VulkanRenderer::DebugCallbackFunction(
	vk::DebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT					messageTypes,
	const vk::DebugUtilsMessengerCallbackDataEXT*		pCallbackData,
	void*												pUserData) {

	return false;
}

bool	VulkanRenderer::MemoryTypeFromPhysicalDeviceProps(vk::MemoryPropertyFlags requirements, uint32_t type, uint32_t& index) {
	for (uint32_t i = 0; i < 32; ++i) {
		if ((type & 1) == 1) {	//We care about this requirement
			if ((m_physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & requirements) == requirements) {
				index = i;
				return true;
			}
		}
		type >>= 1; //Check next bit
	}
	return false;
}

void	VulkanRenderer::CreateDepthBufer(uint32_t width, uint32_t height) {
	m_device.destroyImageView(m_depthView);
	m_device.destroyImage(m_depthImage);
	m_device.freeMemory(m_depthMemory);

	vk::ImageCreateInfo imageCreateInfo = {
		.imageType		= vk::ImageType::e2D,
		.format			= m_vkInit.depthStencilFormat,
		.extent			= {width, height, 1},
		.mipLevels		= 1,
		.arrayLayers	= 1,
		.usage			= vk::ImageUsageFlagBits::eDepthStencilAttachment
	};
	m_depthImage = m_device.createImage(imageCreateInfo);

	vk::MemoryRequirements memReqs = m_device.getImageMemoryRequirements(m_depthImage);

	vk::MemoryAllocateInfo memAllocInfo = {
		.allocationSize = memReqs.size
	};

	bool found = MemoryTypeFromPhysicalDeviceProps({}, memReqs.memoryTypeBits, memAllocInfo.memoryTypeIndex);

	m_depthMemory = m_device.allocateMemory(memAllocInfo);

	m_device.bindImageMemory(m_depthImage, m_depthMemory, 0);

	bool depthStencil = Vulkan::FormatIsDepthStencil(m_vkInit.depthStencilFormat);

	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;

	if (depthStencil) {
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}

	vk::ImageViewCreateInfo viewCreateInfo = {
		.image		= m_depthImage,			
		.viewType	= vk::ImageViewType::e2D,	
		.format		= m_vkInit.depthStencilFormat,
		.subresourceRange = {
			.aspectMask		= aspectFlags,
			.baseMipLevel	= 0,
			.levelCount		= 1,
			.baseArrayLayer = 0,
			.layerCount		= 1,
		}
	};

	m_depthView = m_device.createImageView(viewCreateInfo);

	vk::UniqueCommandBuffer tempBuffer = Vulkan::CmdBufferCreateBegin(m_device, m_commandPools[CommandType::Graphics]);

	vk::ImageMemoryBarrier2 memBarier = {
		.srcStageMask	= vk::PipelineStageFlagBits2::eTopOfPipe,
		.srcAccessMask	= vk::AccessFlagBits2::eNone,

		.dstStageMask	= vk::PipelineStageFlagBits2::eEarlyFragmentTests,
		.dstAccessMask	= vk::AccessFlagBits2::eDepthStencilAttachmentWrite,

		.oldLayout	= vk::ImageLayout::eUndefined,
		.newLayout	= depthStencil ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eDepthAttachmentOptimal,
		.image		= m_depthImage,
		.subresourceRange = {
			.aspectMask		= aspectFlags,
			.baseMipLevel	= 0,
			.levelCount		= 1,
			.baseArrayLayer = 0,
			.layerCount		= 1,
		}
	};

	Vulkan::ImageTransitionBarrier(*tempBuffer, m_depthImage, memBarier);
	
	Vulkan::CmdBufferEndSubmitWait(*tempBuffer, m_device, m_queues[CommandType::Graphics]);

	Vulkan::SetDebugName(m_device, m_depthImage , "DepthBuffer");
	Vulkan::SetDebugName(m_device, m_depthView	, "DepthBuffer");
}