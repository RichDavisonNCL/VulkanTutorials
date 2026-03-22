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

BufferDeviceAddressExample::BufferDeviceAddressExample(Window& window, VulkanInitialisation& vkInit) : VulkanTutorial(window, vkInit){
	m_vkInit.deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	m_vkInit.vmaFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	static vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR deviceAddressfeature = {
		.bufferDeviceAddress = true
	};

	m_vkInit.features.push_back((void*)&deviceAddressfeature);

	Initialise();

	FrameContext const& context = m_renderer->GetFrameContext();

	pointerBuffer = m_memoryManager->CreateBuffer(
		{
				.size = sizeof(vk::DeviceAddress) * 3,
				.usage = vk::BufferUsageFlagBits::eStorageBuffer |
							vk::BufferUsageFlagBits::eShaderDeviceAddress
		},
		vk::MemoryPropertyFlagBits::eHostVisible,
		"Pointer Buffer"
	);

	vk::DeviceAddress* addresses = pointerBuffer.Map<vk::DeviceAddress>();

	for (int i = 0; i < 3; ++i) {
		dataBuffers[i] = m_memoryManager->CreateBuffer(
			{
				.size	= sizeof(Vector4),
				.usage	= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
			},
			vk::MemoryPropertyFlagBits::eHostVisible,
			"Data Buffer " + std::to_string(i)
		);

		Vector4* colour = dataBuffers[i].Map<Vector4>();
		*colour = Vector4(0, 0, 0, 1);
		(*colour)[i] = 1;
		dataBuffers[i].Unmap();

		addresses[i] = dataBuffers[i].deviceAddress;
	}

	pointerBuffer.Unmap();

	pipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m_triangleMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(context.colourFormat)
		.WithDepthAttachment(context.depthFormat)
		.WithShaderBinary("BasicGeometry.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("BufferDeviceAddress.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.Build("Buffer Device Address Pipeline");
}

void BufferDeviceAddressExample::RenderFrame(float dt) {
	FrameContext const& context = m_renderer->GetFrameContext();

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

	context.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, 
		0, sizeof(vk::DeviceAddress), (void*)&pointerBuffer.deviceAddress);
	context.cmdBuffer.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eFragment, 
		sizeof(vk::DeviceAddress), sizeof(uint32_t), (void*)&bufferSelection);

	m_triangleMesh->Draw(context.cmdBuffer);
}