/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "VulkanRenderer.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"
#include "VulkanPipeline.h"
#include "VulkanBuffers.h"
#include "VulkanUtils.h"
#include "VulkanDescriptorSetLayoutBuilder.h"

#include "../../Common/TextureLoader.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include "../../Common/Win32Window.h"
using namespace NCL::Win32Code;
#endif

using namespace NCL;
using namespace Rendering;

vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;

VulkanRenderer::VulkanRenderer(Window& window, VulkanInitInfo info) : RendererBase(window), initInfo(info) {
	depthBuffer		= nullptr;
	frameBuffers	= nullptr;

	extensionList.insert(extensionList.end(), info.extensions.begin(), info.extensions.end());
	layerList.insert(layerList.end(), info.layers.begin(), info.layers.end());

	InitInstance(info.majorVersion, info.minorVersion);

	InitPhysicalDevice();
	
	InitGPUDevice();

	InitCommandPools();
	InitDefaultDescriptorPool();

	VulkanTexture::SetRenderer(this);
	TextureLoader::RegisterAPILoadFunction(VulkanTexture::TextureFromFilenameLoader);

	window.SetRenderer(this);	
	OnWindowResize((int)hostWindow.GetScreenSize().x, (int)hostWindow.GetScreenSize().y);

	pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	defaultCmdBuffer = swapChainList[currentSwap]->frameCmds;
}

VulkanRenderer::~VulkanRenderer() {
	depthBuffer.reset();

	for (auto& i : swapChainList) {
		device.destroyImageView(i->view);
	};

	for (unsigned int i = 0; i < numFrameBuffers; ++i) {
		device.destroyFramebuffer(frameBuffers[i]);
	}
	device.destroyDescriptorPool(defaultDescriptorPool);
	device.destroySwapchainKHR(swapChain);
	device.destroyCommandPool(commandPool);
	device.destroyCommandPool(computeCommandPool);
	device.destroyRenderPass(defaultRenderPass);
	device.destroyPipelineCache(pipelineCache);
	device.destroy(); //Destroy everything except instance before this gets destroyed!

	delete Vulkan::dispatcher;

	instance.destroySurfaceKHR(surface);
	instance.destroy();

	delete[] frameBuffers;
}

bool VulkanRenderer::InitInstance(int major, int minor) {
	vk::ApplicationInfo appInfo = vk::ApplicationInfo(this->hostWindow.GetTitle().c_str());

	appInfo.apiVersion = VK_MAKE_VERSION(major, minor, 0);

	vector<const char*> instanceExtensions;
	vector<const char*> instanceLayers;

	instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef WIN32
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	instanceLayers.push_back("VK_LAYER_KHRONOS_validation");

	vk::InstanceCreateInfo instanceInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo)
		.setEnabledExtensionCount((uint32_t)instanceExtensions.size())
		.setPpEnabledExtensionNames(instanceExtensions.data())
		.setEnabledLayerCount((uint32_t)instanceLayers.size())
		.setPpEnabledLayerNames(instanceLayers.data());

	instance = vk::createInstance(instanceInfo);
	Vulkan::dispatcher = new vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr); //Instance dispatcher
	return true;
}

bool	VulkanRenderer::InitPhysicalDevice() {
	auto enumResult = instance.enumeratePhysicalDevices();

	if (enumResult.empty()) {
		return false; //Guess there's no Vulkan capable devices?!
	}

	gpu = enumResult[0];
	for (auto& i : enumResult) {
		if (i.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			gpu = i; //Prefer a discrete GPU on multi device machines like laptops
		}
	}

	std::cout << __FUNCTION__ << " Vulkan using physical device " << gpu.getProperties().deviceName << std::endl;

	return true;
}

bool VulkanRenderer::InitGPUDevice() {
	extensionList.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	extensionList.push_back("VK_KHR_dynamic_rendering");
	extensionList.push_back("VK_KHR_maintenance4");

	extensionList.push_back("VK_KHR_depth_stencil_resolve");
	extensionList.push_back("VK_KHR_create_renderpass2");
	layerList.push_back("VK_LAYER_LUNARG_standard_validation");

	float queuePriority = 0.0f;
	vk::DeviceQueueCreateInfo queueInfo = vk::DeviceQueueCreateInfo()
		.setQueueCount(1)
		.setQueueFamilyIndex(gfxQueueIndex)
		.setPQueuePriorities(&queuePriority);

	vk::PhysicalDeviceFeatures features = vk::PhysicalDeviceFeatures()
		.setMultiDrawIndirect(true)
		.setDrawIndirectFirstInstance(true)
		.setShaderClipDistance(true)
		.setShaderCullDistance(true);


	vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering;
	dynamicRendering.dynamicRendering = true;
	
	vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPNext(&dynamicRendering)
		.setPQueueCreateInfos(&queueInfo)
		.setPEnabledFeatures(&features)
		.setEnabledLayerCount((uint32_t)layerList.size())
		.setPpEnabledLayerNames(layerList.data())
		.setEnabledExtensionCount((uint32_t)extensionList.size())
		.setPpEnabledExtensionNames(extensionList.data());

	if (initInfo.deviceModifier) {
		initInfo.deviceModifier(createInfo, gpu);
	}

	InitSurface();
	InitDeviceQueues();

	device = gpu.createDevice(createInfo);
	deviceQueue = device.getQueue(gfxQueueIndex, 0);
	deviceMemoryProperties = gpu.getMemoryProperties();

	delete Vulkan::dispatcher;
	Vulkan::dispatcher = new vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr , device); //DEVICE dispatcher
	return true;
}

bool VulkanRenderer::InitSurface() {
#ifdef _WIN32
	Win32Window* window = (Win32Window*)&hostWindow;

	vk::Win32SurfaceCreateInfoKHR createInfo;

	createInfo = vk::Win32SurfaceCreateInfoKHR(
		vk::Win32SurfaceCreateFlagsKHR(), window->GetInstance(), window->GetHandle());

	surface = instance.createWin32SurfaceKHR(createInfo);
#endif

	auto formats = gpu.getSurfaceFormatsKHR(surface);

	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
		surfaceFormat	= vk::Format::eB8G8R8A8Unorm;
		surfaceSpace	= formats[0].colorSpace;
	}
	else {
		surfaceFormat	= formats[0].format;
		surfaceSpace	= formats[0].colorSpace;
	}

	return formats.size() > 0;
}

uint32_t VulkanRenderer::InitBufferChain(vk::CommandBuffer  cmdBuffer) {
	vk::SwapchainKHR oldChain					= swapChain;
	std::vector<SwapChain*> oldSwapChainList	= swapChainList;
	swapChainList.clear();

	vk::SurfaceCapabilitiesKHR surfaceCaps = gpu.getSurfaceCapabilitiesKHR(surface);

	vk::Extent2D swapExtents = vk::Extent2D((int)hostWindow.GetScreenSize().x, (int)hostWindow.GetScreenSize().y);

	auto presentModes = gpu.getSurfacePresentModesKHR(surface); //Type is of vector of PresentModeKHR

	vk::PresentModeKHR idealPresentMode = vk::PresentModeKHR::eFifo;

	for (const auto& i : presentModes) {
		if (i == vk::PresentModeKHR::eMailbox) {
			idealPresentMode = i;
			break;
		}
		else if (i == vk::PresentModeKHR::eImmediate) {
			idealPresentMode = vk::PresentModeKHR::eImmediate; //Might still become mailbox...
		}
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

	swapInfo.setPresentMode(idealPresentMode)
		.setPreTransform(idealTransform)
		.setSurface(surface)
		.setImageColorSpace(surfaceSpace)
		.setImageFormat(surfaceFormat)
		.setImageExtent(swapExtents)
		.setMinImageCount(idealImageCount)
		.setOldSwapchain(oldChain)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	swapChain = device.createSwapchainKHR(swapInfo);

	if (!oldSwapChainList.empty()) {
		for (unsigned int i = 0; i < numFrameBuffers; ++i) {
			device.destroyImageView(oldSwapChainList[i]->view);
			delete oldSwapChainList[i];
		}
	}
	if (oldChain) {
		device.destroySwapchainKHR(oldChain);
	}

	auto images = device.getSwapchainImagesKHR(swapChain);

	for (auto& i : images) {
		vk::ImageViewCreateInfo viewCreate = vk::ImageViewCreateInfo()
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
			.setFormat(surfaceFormat)
			.setImage(i)
			.setViewType(vk::ImageViewType::e2D);

		SwapChain* chain = new SwapChain();

		chain->image = i;

		ImageTransitionBarrier(cmdBuffer, i, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput);

		chain->view = device.createImageView(viewCreate);

		swapChainList.push_back(chain);

		auto buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
			commandPool, vk::CommandBufferLevel::ePrimary, 1));

		chain->frameCmds = buffers[0];
	}

	return (int)images.size();
}

void	VulkanRenderer::ImageTransitionBarrier(vk::CommandBuffer  cmdBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage, int mipLevel, int layer) {
	vk::ImageSubresourceRange subRange = vk::ImageSubresourceRange(aspect, mipLevel, 1, layer, 1);

	vk::ImageMemoryBarrier memoryBarrier = vk::ImageMemoryBarrier()
		.setSubresourceRange(subRange)
		.setImage(image)
		.setOldLayout(oldLayout)
		.setNewLayout(newLayout);

	if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	}
	else if (newLayout == vk::ImageLayout::eTransferSrcOptimal) {
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	}
	else if (newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	}
	else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	}
	else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead; //added last bit?!?
	}

	cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

void	VulkanRenderer::ImageTransitionBarrier(vk::CommandBuffer  buffer, const VulkanTexture* t, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage, int mipLevel, int layer) {
	ImageTransitionBarrier(buffer, t->GetImage(), oldLayout, newLayout, aspect, srcStage, dstStage, mipLevel, layer);
}

void	VulkanRenderer::InitCommandPools() {
	computeCommandPool = device.createCommandPool(vk::CommandPoolCreateInfo(
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer, computeQueueIndex));

	commandPool = device.createCommandPool(vk::CommandPoolCreateInfo(
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer, gfxQueueIndex));
}

vk::CommandBuffer VulkanRenderer::BeginComputeCmdBuffer(const std::string& debugName) {
	vk::CommandBufferAllocateInfo bufferInfo = vk::CommandBufferAllocateInfo(computeCommandPool, vk::CommandBufferLevel::ePrimary, 1);

	auto buffers = device.allocateCommandBuffers(bufferInfo); //Func returns a vector!

	vk::CommandBuffer  newBuf = buffers[0];

	vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo();

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eCommandBuffer, Vulkan::GetVulkanHandle(newBuf), debugName);
	}

	newBuf.begin(beginInfo);
	return newBuf;
}

vk::CommandBuffer VulkanRenderer::BeginCmdBuffer(const std::string& debugName) {
	vk::CommandBufferAllocateInfo bufferInfo = vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

	auto buffers = device.allocateCommandBuffers(bufferInfo); //Func returns a vector!

	vk::CommandBuffer &newBuf = buffers[0];

	vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo();

	newBuf.begin(beginInfo);
	newBuf.setViewport(0, 1, &defaultViewport);
	newBuf.setScissor( 0, 1, &defaultScissor);

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, vk::ObjectType::eCommandBuffer, Vulkan::GetVulkanHandle(newBuf), debugName);
	}

	return newBuf;
}

void		VulkanRenderer::SubmitCmdBufferWait(vk::CommandBuffer  buffer) {
	vk::Fence fence = SubmitCmdBufferFence(buffer);

	if (!fence) {
		return;
	}

	if (device.waitForFences(1, &fence, true, UINT64_MAX) != vk::Result::eSuccess) {
		std::cout << __FUNCTION__ << " Device queue submission taking too long?\n";
	};

	device.destroyFence(fence);
}

void 	VulkanRenderer::SubmitCmdBuffer(vk::CommandBuffer  buffer) {
	if (buffer) {	
		buffer.end();
	}
	else {
		std::cout << __FUNCTION__ << " Submitting invalid buffer?\n";
		return;
	}
	vk::SubmitInfo submitInfo = vk::SubmitInfo();
	submitInfo.setCommandBufferCount(1);
	submitInfo.setPCommandBuffers(&buffer);

	deviceQueue.submit(submitInfo, {});
}

vk::Fence 	VulkanRenderer::SubmitCmdBufferFence(vk::CommandBuffer  buffer) {
	vk::Fence fence;
	if (buffer) {
		
		buffer.end();
	}
	else {
		std::cout << __FUNCTION__ << " Submitting invalid buffer?\n";
		return fence;
	}
	fence = device.createFence(vk::FenceCreateInfo());

	vk::SubmitInfo submitInfo = vk::SubmitInfo();
	submitInfo.setCommandBufferCount(1);
	submitInfo.setPCommandBuffers(&buffer);

	deviceQueue.submit(submitInfo, fence);

	return fence;
}

bool VulkanRenderer::InitDeviceQueues() {
	vector<vk::QueueFamilyProperties> deviceQueueProps = gpu.getQueueFamilyProperties();

	VkBool32 supportsPresent = false;
	gfxQueueIndex		= -1;
	gfxPresentIndex		= -1;
	computeQueueIndex	= -1;

	for (unsigned int i = 0; i < deviceQueueProps.size(); ++i) {
		supportsPresent = gpu.getSurfaceSupportKHR(i, surface);

		if (computeQueueIndex == -1 && deviceQueueProps[i].queueFlags & vk::QueueFlagBits::eCompute) {
			computeQueueIndex = i;
		}

		if (deviceQueueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			gfxQueueIndex = i;
			if (supportsPresent && gfxPresentIndex == -1) {
				gfxPresentIndex = i;
			}
		}
	}

	if (gfxPresentIndex == -1) {
		for (unsigned int i = 0; i < deviceQueueProps.size(); ++i) {
			supportsPresent = gpu.getSurfaceSupportKHR(i, surface);
			if (supportsPresent) {
				gfxPresentIndex = i;
				break;
			}
		}
	}

	if (gfxQueueIndex == -1 || gfxPresentIndex == -1 || computeQueueIndex == -1) {
		return false;
	}

	return true;
}

void VulkanRenderer::OnWindowResize(int width, int height) {
	if (!hostWindow.IsMinimised() && width == windowWidth && height == windowHeight) {
		return;
	}
	if (width == 0 && height == 0) {
		return;
	}
	windowWidth		= width;
	windowHeight	= height;

	defaultScreenRect = vk::Rect2D({ 0,0 }, { (uint32_t)windowWidth, (uint32_t)windowHeight });

	defaultViewport = vk::Viewport(0.0f, (float)windowHeight, (float)windowWidth, -(float)windowHeight, 0.0f, 1.0f);
	defaultScissor	= vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(windowWidth, windowHeight));

	defaultClearValues[0] = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}));
	defaultClearValues[1] = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

	vk::CommandBuffer cmds = BeginCmdBuffer();

	std::cout << __FUNCTION__ << " New dimensions: " << windowWidth << " , " << windowHeight << "\n";
	vkDeviceWaitIdle(device);

	//delete depthBuffer;
	depthBuffer = VulkanTexture::CreateDepthTexture((int)hostWindow.GetScreenSize().x, (int)hostWindow.GetScreenSize().y);
	
	numFrameBuffers = InitBufferChain(cmds);

	InitDefaultRenderPass();
	CreateDefaultFrameBuffers();

	vkDeviceWaitIdle(device);

	vk::Semaphore	presentSempaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
	vk::Fence		fence = device.createFence(vk::FenceCreateInfo());

	currentSwap = device.acquireNextImageKHR(swapChain, UINT64_MAX, presentSempaphore, vk::Fence()).value;	//Get swap image

	device.destroySemaphore(presentSempaphore);
	device.destroyFence(fence);

	CompleteResize();

	SubmitCmdBufferWait(cmds);
}

void VulkanRenderer::CompleteResize() {

}

void	VulkanRenderer::BeginFrame() {
	//if (hostWindow.IsMinimised()) {
	//	defaultCmdBuffer = BeginCmdBuffer();
	//}
	defaultCmdBuffer.reset({});

	defaultCmdBuffer.begin(vk::CommandBufferBeginInfo());
	defaultCmdBuffer.setViewport(0, 1, &defaultViewport);
	defaultCmdBuffer.setScissor(0, 1, &defaultScissor);
}

void	VulkanRenderer::EndFrame() {
	if (hostWindow.IsMinimised()) {
		SubmitCmdBufferWait(defaultCmdBuffer);
	}
	else {
		SubmitCmdBuffer(defaultCmdBuffer);
	}
}

void VulkanRenderer::SwapBuffers() {
	vk::UniqueSemaphore	presentSempaphore;
	vk::UniqueFence		presentFence;

	if (!hostWindow.IsMinimised()) {
		vk::CommandBuffer cmds = BeginCmdBuffer();
		TransitionSwapchainForPresenting(cmds);
		SubmitCmdBufferWait(cmds);
		device.freeCommandBuffers(commandPool, cmds);

		vk::Result presentResult = deviceQueue.presentKHR(vk::PresentInfoKHR(0, nullptr, 1, &swapChain, &currentSwap, nullptr));

		presentSempaphore = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		presentFence	  = device.createFenceUnique(vk::FenceCreateInfo());
	
		currentSwap = device.acquireNextImageKHR(swapChain, UINT64_MAX, *presentSempaphore, *presentFence).value;	//Get swap image
	}
	defaultBeginInfo = vk::RenderPassBeginInfo()
		.setRenderPass(defaultRenderPass)
		.setFramebuffer(frameBuffers[currentSwap])
		.setRenderArea(defaultScissor)
		.setClearValueCount(sizeof(defaultClearValues) / sizeof(vk::ClearValue))
		.setPClearValues(defaultClearValues);

	defaultCmdBuffer = swapChainList[currentSwap]->frameCmds;

	if (!hostWindow.IsMinimised()) {
		vk::Result waitResult = device.waitForFences(*presentFence, true, ~0);
	}
}

void	VulkanRenderer::InitDefaultRenderPass() {
	if (defaultRenderPass) {
		device.destroyRenderPass(defaultRenderPass);
	}
	vk::AttachmentDescription attachments[] = {
		vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFormat(surfaceFormat)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	,
		vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFormat(depthBuffer->GetFormat())
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	};

	vk::AttachmentReference references[] = {
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
	};

	vk::SubpassDescription subPass = vk::SubpassDescription()
		.setColorAttachmentCount(1)
		.setPColorAttachments(&references[0])
		.setPDepthStencilAttachment(&references[1])
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(2)
		.setPAttachments(attachments)
		.setSubpassCount(1)
		.setPSubpasses(&subPass);

	defaultRenderPass = device.createRenderPass(renderPassInfo);
}

bool VulkanRenderer::CreateDefaultFrameBuffers() {
	if (frameBuffers) {
		for (unsigned int i = 0; i < numFrameBuffers; ++i) {
			device.destroyFramebuffer(frameBuffers[i]);
		}
	}
	else {
		frameBuffers = new vk::Framebuffer[numFrameBuffers];
	}

	vk::ImageView attachments[2];
	
	vk::FramebufferCreateInfo createInfo = vk::FramebufferCreateInfo()
		.setWidth((int)hostWindow.GetScreenSize().x)
		.setHeight((int)hostWindow.GetScreenSize().y)
		.setLayers(1)
		.setAttachmentCount(2)
		.setPAttachments(attachments)
		.setRenderPass(defaultRenderPass);

	for (uint32_t i = 0; i < numFrameBuffers; ++i) {
		attachments[0]	= swapChainList[i]->view;
		attachments[1]	= *depthBuffer->defaultView;
		frameBuffers[i] = device.createFramebuffer(createInfo);
	}

	defaultBeginInfo = vk::RenderPassBeginInfo()
		.setRenderPass(defaultRenderPass)
		.setFramebuffer(frameBuffers[currentSwap])
		.setRenderArea(defaultScissor)
		.setClearValueCount(sizeof(defaultClearValues) / sizeof(vk::ClearValue))
		.setPClearValues(defaultClearValues);

	return true;
}

bool	VulkanRenderer::MemoryTypeFromPhysicalDeviceProps(vk::MemoryPropertyFlags requirements, uint32_t type, uint32_t& index) {
	for (uint32_t i = 0; i < 32; ++i) {
		if ((type & 1) == 1) {	//We care about this requirement
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & requirements) == requirements) {
				index = i;
				return true;
			}
		}
		type >>= 1; //Check next bit
	}
	return false;
}

void	VulkanRenderer::InitDefaultDescriptorPool() {
	int maxSets = 128; //how many times can we ask the pool for a descriptor set?
	vk::DescriptorPoolSize poolSizes[] = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 128),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 128),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 128)
	};

	vk::DescriptorPoolCreateInfo poolCreate;
	poolCreate.setPoolSizeCount(sizeof(poolSizes) / sizeof(vk::DescriptorPoolSize));
	poolCreate.setPPoolSizes(poolSizes);
	poolCreate.setMaxSets(maxSets);
	poolCreate.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	defaultDescriptorPool = device.createDescriptorPool(poolCreate);
}

void	VulkanRenderer::UpdateImageDescriptor(vk::DescriptorSet set, int bindingNum, int subIndex, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout) {
	auto imageInfo = vk::DescriptorImageInfo()
		.setSampler(sampler)
		.setImageView(view)
		.setImageLayout(layout);

	auto descriptorWrite = vk::WriteDescriptorSet()
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDstSet(set)
		.setDstBinding(bindingNum)
		.setDstArrayElement(subIndex)
		.setDescriptorCount(1)
		.setPImageInfo(&imageInfo);

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void VulkanRenderer::UpdateBufferDescriptor(vk::DescriptorSet set, const VulkanBuffer& data, int bindingSlot, vk::DescriptorType bufferType) {
	auto descriptorInfo = vk::DescriptorBufferInfo()
		.setBuffer(*(data.buffer))
		.setRange(data.requestedSize);
	
	auto descriptorWrites = vk::WriteDescriptorSet()
		.setDescriptorType(bufferType)
		.setDstSet(set)
		.setDstBinding(bindingSlot)
		.setDescriptorCount(1)
		.setPBufferInfo(&descriptorInfo);

	device.updateDescriptorSets(1, &descriptorWrites, 0, nullptr);
}

VulkanBuffer VulkanRenderer::CreateBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
	VulkanBuffer buffer;
	buffer.requestedSize = size;

	buffer.buffer = device.createBufferUnique(vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage));

	vk::MemoryRequirements reqs = device.getBufferMemoryRequirements(*buffer.buffer);
	buffer.allocInfo = vk::MemoryAllocateInfo(reqs.size);

	bool found = MemoryTypeFromPhysicalDeviceProps(properties, reqs.memoryTypeBits, buffer.allocInfo.memoryTypeIndex);

	buffer.deviceMem = device.allocateMemoryUnique(buffer.allocInfo);

	device.bindBufferMemory(*buffer.buffer, *buffer.deviceMem, 0);

	return buffer;
}

void VulkanRenderer::DestroyBuffer(VulkanBuffer& buffer) {
	buffer.buffer.reset();
	buffer.deviceMem.reset();
}

void VulkanRenderer::UploadBufferData(VulkanBuffer& uniform, void* data, int dataSize) {
	void* mappedData = device.mapMemory(*uniform.deviceMem, 0, uniform.allocInfo.allocationSize);
	memcpy(mappedData, data, dataSize);
	device.unmapMemory(*uniform.deviceMem);
}

vk::UniqueDescriptorSet VulkanRenderer::BuildUniqueDescriptorSet(vk::DescriptorSetLayout  layout, vk::DescriptorPool pool, uint32_t variableDescriptorCount) {
	if (!pool) {
		pool = defaultDescriptorPool;
	}
	vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(pool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&layout);

	vk::DescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorInfo;

	if (variableDescriptorCount > 0) {
		variableDescriptorInfo.setDescriptorSetCount(1);
		variableDescriptorInfo.pDescriptorCounts = &variableDescriptorCount;
		allocateInfo.setPNext((const void*)&variableDescriptorInfo);
	}

	return std::move(device.allocateDescriptorSetsUnique(allocateInfo)[0]);
}

void VulkanRenderer::SubmitDrawCallLayer(const VulkanMesh& m, unsigned int layer, vk::CommandBuffer  to, int instanceCount) {
	VkDeviceSize baseOffset = 0;

	const SubMesh* sm = m.GetSubMesh(layer);

	m.BindToCommandBuffer(to);

	if (m.GetIndexCount() > 0) {
		to.drawIndexed(sm->count, instanceCount, sm->start, sm->base, 0);
	}
	else {
		to.draw(sm->count, instanceCount, sm->start, 0);
	}
}


void VulkanRenderer::SubmitDrawCall(const VulkanMesh& m, vk::CommandBuffer  to, int instanceCount) {
	VkDeviceSize baseOffset = 0;

	m.BindToCommandBuffer(to);

	if (m.GetIndexCount() > 0) {
		to.drawIndexed(m.GetIndexCount(), instanceCount, 0, 0, 0);
	}
	else {
		to.draw(m.GetVertexCount(), instanceCount, 0, 0);
	}
}

void VulkanRenderer::DispatchCompute(vk::CommandBuffer  to, unsigned int xCount, unsigned int yCount, unsigned int zCount) {
	to.dispatch(xCount, yCount, zCount);
}

bool VulkanRenderer::EnableRayTracing() {
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR	pipeProperties;
	vk::PhysicalDeviceAccelerationStructureFeaturesKHR	accelFeatures;

	vk::PhysicalDeviceProperties2 props;
	props.pNext = &pipeProperties;

	gpu.getProperties2(&props, *Vulkan::dispatcher);
	//gpu.getFeatures2KHR(accelFeatures);

	auto properties =
		gpu.getProperties2<vk::PhysicalDeviceProperties2,
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

	return true;
}

void	VulkanRenderer::BeginDefaultRenderPass(vk::CommandBuffer  cmds) {
	cmds.beginRenderPass(defaultBeginInfo, vk::SubpassContents::eInline);
	cmds.setViewport(0, 1, &defaultViewport);
	cmds.setScissor(0, 1, &defaultScissor);
}

void VulkanRenderer::TransitionSwapchainForRendering(vk::CommandBuffer buffer) {
	ImageTransitionBarrier(buffer, swapChainList[currentSwap]->image,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageAspectFlagBits::eColor, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

void VulkanRenderer::TransitionSwapchainForPresenting(vk::CommandBuffer buffer) {
	ImageTransitionBarrier(buffer, swapChainList[currentSwap]->image,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::ImageAspectFlagBits::eColor, vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlagBits::eBottomOfPipe);
}

void	VulkanRenderer::BeginDefaultRendering(vk::CommandBuffer  cmds) {
	vk::RenderingInfoKHR renderInfo;
	renderInfo.layerCount = 1;

	vk::RenderingAttachmentInfoKHR colourAttachment;
	colourAttachment.setImageView(swapChainList[currentSwap]->view)
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setClearValue(ClearColour(0.2f, 0.2f, 0.2f, 1.0f));

	vk::RenderingAttachmentInfoKHR depthAttachment;
	depthAttachment.setImageView(depthBuffer->GetDefaultView())
		.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare);
	depthAttachment.clearValue.setDepthStencil({ 1.0f, ~0U });

	renderInfo.setColorAttachments(colourAttachment)
		.setPDepthAttachment(&depthAttachment)
		.setPStencilAttachment(&depthAttachment);

	renderInfo.setRenderArea(defaultScreenRect);

	cmds.beginRendering(renderInfo, *NCL::Rendering::Vulkan::dispatcher);
}

void	VulkanRenderer::EndRendering(vk::CommandBuffer  cmds) {
	cmds.endRendering(*NCL::Rendering::Vulkan::dispatcher);
}