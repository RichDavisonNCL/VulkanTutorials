/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "BufferDeviceAddressExample.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

TUTORIAL_ENTRY(BufferDeviceAddressExample)

BufferDeviceAddressExample::BufferDeviceAddressExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window){
	vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
	};

	//static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {
	//	.runtimeDescriptorArray = true
	//};

	vkInit.features.push_back((void*)&deviceAddressfeature);
//	vkInit.features.push_back((void*)&indexingFeatures);

	renderer = new VulkanRenderer(window, vkInit);
	InitTutorialObjects();

	FrameState const& frameState = renderer->GetFrameState();
	vk::Device device = renderer->GetDevice();
	vk::DescriptorPool pool = renderer->GetDescriptorPool();

	triMesh = GenerateTriangle();

	shader = ShaderBuilder(device)
		.WithVertexBinary("BasicGeometry.vert.spv")
		.WithFragmentBinary("BufferDeviceAddress.frag.spv")
		.Build("Buffer Device Address Shader!");

	pointerBuffer = BufferBuilder(device, renderer->GetMemoryAllocator())
		.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.WithDeviceAddress()
		.WithHostVisibility()
		.Build(sizeof(vk::DeviceAddress) * 3, "Pointer Buffer");

	vk::DeviceAddress* addresses = pointerBuffer.Map<vk::DeviceAddress>();

	for (int i = 0; i < 3; ++i) {
		dataBuffers[i] = BufferBuilder(device, renderer->GetMemoryAllocator())
			.WithBufferUsage(vk::BufferUsageFlagBits::eStorageBuffer)
			.WithDeviceAddress()
			.WithHostVisibility()
			.Build(sizeof(Vector4), "Data Buffer " + std::to_string(i));

		Vector4* colour = dataBuffers[i].Map<Vector4>();
		*colour = Vector4(0, 0, 0, 1);
		(*colour)[i] = 1;
		dataBuffers[i].Unmap();

		addresses[i] = dataBuffers[i].deviceAddress;
	}

	pointerBuffer.Unmap();

	pipeline = PipelineBuilder(device)
		.WithVertexInputState(triMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(frameState.colourFormat)
		.WithDepthAttachment(frameState.depthFormat)
		.WithShader(shader)
		.Build("Buffer Device Address Pipeline");
}

void BufferDeviceAddressExample::RenderFrame(float dt) {
	FrameState const& state = renderer->GetFrameState();

	state.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	static int bufferSelection = 0;
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1)) {
		bufferSelection = 0;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2)) {
		bufferSelection = 1;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM3)) {
		bufferSelection = 2;
	}

	state.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(vk::DeviceAddress), (void*)&pointerBuffer.deviceAddress);
	state.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, sizeof(vk::DeviceAddress), sizeof(uint32_t), (void*)&bufferSelection);

	triMesh->Draw(state.cmdBuffer);
}