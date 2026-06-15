/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanUtils.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "Assets.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

std::map<vk::Device, vk::DescriptorSetLayout > nullDescriptors;

vk::detail::DynamicLoader NCL::Rendering::Vulkan::dynamicLoader;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Vulkan::BeginDebugArea(vk::CommandBuffer b, const std::string& name) {
	vk::DebugUtilsLabelEXT labelInfo;
	labelInfo.pLabelName = name.c_str();
	b.beginDebugUtilsLabelEXT(labelInfo);
}

void Vulkan::EndDebugArea(vk::CommandBuffer b) {
	b.endDebugUtilsLabelEXT();
}

void Vulkan::SetNullDescriptor(vk::Device device, vk::DescriptorSetLayout layout) {
	nullDescriptors.insert({ device, layout });
}

vk::DescriptorSetLayout Vulkan::GetNullDescriptor(vk::Device device) {
	return nullDescriptors[device];
}

vk::AccessFlags Vulkan::DefaultAccessFlags(vk::ImageLayout forLayout) {
	if (forLayout == vk::ImageLayout::eTransferDstOptimal) {
		return vk::AccessFlagBits::eTransferWrite;
	}
	else if (forLayout == vk::ImageLayout::eTransferSrcOptimal) {
		return vk::AccessFlagBits::eTransferRead;
	}
	else if (forLayout == vk::ImageLayout::eColorAttachmentOptimal) {
		return vk::AccessFlagBits::eColorAttachmentWrite;
	}
	else if (forLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	}
	else if (forLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead; //added last bit?!?
	}
	return vk::AccessFlagBits::eNone;
}

vk::AccessFlags2 Vulkan::DefaultAccessFlags2(vk::ImageLayout forLayout) {
	if (forLayout == vk::ImageLayout::eTransferDstOptimal) {
		return vk::AccessFlagBits2::eTransferWrite;
	}
	else if (forLayout == vk::ImageLayout::eTransferSrcOptimal) {
		return vk::AccessFlagBits2::eTransferRead;
	}
	else if (forLayout == vk::ImageLayout::eColorAttachmentOptimal) {
		return vk::AccessFlagBits2::eColorAttachmentWrite;
	}
	else if (forLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	}
	else if (forLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eInputAttachmentRead; //added last bit?!?
	}
	return vk::AccessFlagBits2::eNone;
}

void Vulkan::ImageTransitionBarrier(vk::CommandBuffer  buffer, vk::Image i, vk::ImageMemoryBarrier2 barrier) {
	barrier.image = i;
	buffer.pipelineBarrier2({
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier
	});
}

void	Vulkan::ImageTransitionBarrier(vk::CommandBuffer  cmdBuffer, vk::Image image, 
	vk::ImageLayout oldLayout, vk::ImageLayout newLayout, 
	vk::ImageAspectFlags aspect, 
	vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage, 
	uint32_t mipLevel, uint32_t mipCount, uint32_t layer, uint32_t layerCount) {

	vk::ImageMemoryBarrier2 memoryBarrier2 = {
		.srcStageMask	= srcStage,	
		.dstStageMask	= dstStage,
		.dstAccessMask	= DefaultAccessFlags2(newLayout),
		.oldLayout		= oldLayout,
		.newLayout		= newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image			= image,
		.subresourceRange = {
			.aspectMask		= aspect,
			.baseMipLevel	= mipLevel,
			.levelCount		= mipCount,
			.baseArrayLayer = layer,
			.layerCount		= layerCount,
		}
	};	
	cmdBuffer.pipelineBarrier2({
		.imageMemoryBarrierCount	= 1,
		.pImageMemoryBarriers		= &memoryBarrier2
	});
}

void Vulkan::TransitionUndefinedToColour(vk::CommandBuffer  buffer, vk::Image t) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageAspectFlagBits::eColor, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput);
}

void Vulkan::TransitionColourToPresent(vk::CommandBuffer  buffer, vk::Image t) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eBottomOfPipe);
}

void Vulkan::TransitionColourToSampler(vk::CommandBuffer  buffer, vk::Image t) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eFragmentShader);
}

void Vulkan::TransitionSamplerToColour(vk::CommandBuffer  buffer, vk::Image t) {
	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor,
		vk::PipelineStageFlagBits2::eFragmentShader, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
}

void Vulkan::TransitionDepthToSampler(vk::CommandBuffer  buffer, vk::Image t, bool doStencil) {
	vk::ImageAspectFlags flags = doStencil ? vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eDepth;

	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal, flags,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests, vk::PipelineStageFlagBits2::eFragmentShader);
}

void Vulkan::TransitionSamplerToDepth(vk::CommandBuffer  buffer, vk::Image t, bool doStencil) {
	vk::ImageAspectFlags flags = doStencil ? vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eDepth;

	ImageTransitionBarrier(buffer, t,
		vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal, flags,
		vk::PipelineStageFlagBits2::eFragmentShader, vk::PipelineStageFlagBits2::eEarlyFragmentTests);
}

bool Vulkan::MessageAssert(bool condition, const char* msg) {
	if (!condition) {
		std::cerr << msg << "\n";
	}
	return condition;
}

vk::UniqueCommandBuffer	Vulkan::CmdBufferCreate(vk::Device device, vk::CommandPool fromPool, const std::string& debugName) {
	std::vector<vk::UniqueCommandBuffer> buffers = device.allocateCommandBuffersUnique(
		{
			.commandPool = fromPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1
		}
	);

	if (!debugName.empty()) {
		Vulkan::SetDebugName(device, *buffers[0], debugName);
	}
	return std::move(buffers[0]);
}

void	Vulkan::CmdBufferResetBegin(vk::CommandBuffer  buffer) {
	buffer.reset();
	buffer.begin(vk::CommandBufferBeginInfo());
}

void	Vulkan::CmdBufferResetBegin(const vk::UniqueCommandBuffer& buffer) {
	buffer->reset();
	buffer->begin(vk::CommandBufferBeginInfo());
}

vk::UniqueCommandBuffer	Vulkan::CmdBufferCreateBegin(vk::Device device, vk::CommandPool fromPool, const std::string& debugName) {
	vk::UniqueCommandBuffer buffer = CmdBufferCreate(device, fromPool, debugName);
	vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo();
	buffer->begin(beginInfo);
	return std::move(buffer);
}

void	Vulkan::CmdBufferEndSubmit(vk::CommandBuffer  buffer, vk::Queue queue, vk::Fence fence, vk::Semaphore waitSemaphore, vk::Semaphore signalSempahore) {
	if (!buffer) {
		std::cout << __FUNCTION__ << " Submitting invalid buffer?\n";
		return;
	}
	buffer.end();

	vk::SubmitInfo submitInfo = {
		.commandBufferCount = 1,
		.pCommandBuffers = &buffer
	};
		
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTopOfPipe;

	if (waitSemaphore) {
		submitInfo.waitSemaphoreCount	= 1;
		submitInfo.pWaitSemaphores		= &waitSemaphore;
		submitInfo.pWaitDstStageMask	= &waitStage;
	}
	if (signalSempahore) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= &signalSempahore;
	}

	queue.submit(submitInfo, fence);
}

void		Vulkan::CmdBufferEndSubmitWait(vk::CommandBuffer  buffer, vk::Device device, vk::Queue queue) {
	vk::Fence fence = device.createFence({});

	CmdBufferEndSubmit(buffer, queue, fence);

	if (device.waitForFences(1, &fence, true, UINT64_MAX) != vk::Result::eSuccess) {
		std::cout << __FUNCTION__ << " Device queue submission taking too long?\n";
	};

	device.destroyFence(fence);
}

void	Vulkan::CmdBufferEndSubmitWait(vk::CommandBuffer  buffer, vk::Device device, vk::Queue queue, vk::Fence fence) {
	CmdBufferEndSubmit(buffer, queue, fence);

	if (device.waitForFences(1, &fence, true, UINT64_MAX) != vk::Result::eSuccess) {
		std::cout << __FUNCTION__ << " Device queue submission taking too long?\n";
	};
}

void	Vulkan::WriteDescriptor(vk::Device device, vk::WriteDescriptorSet setInfo, vk::DescriptorBufferInfo bufferInfo) {
	setInfo.descriptorCount = 1;
	setInfo.pBufferInfo = &bufferInfo;
	if (bufferInfo.range == 0) {
		bufferInfo.range = VK_WHOLE_SIZE;
	}
	device.updateDescriptorSets(1, &setInfo, 0, nullptr);
}

void	Vulkan::WriteDescriptor(vk::Device device, vk::WriteDescriptorSet setInfo, vk::DescriptorImageInfo imageInfo) {
	setInfo.pImageInfo = &imageInfo;
	device.updateDescriptorSets(1, &setInfo, 0, nullptr);
}

void	Vulkan::WriteCombinedImageDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingNum, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout) {
	vk::DescriptorImageInfo imageInfo = {
		.sampler		= sampler,
		.imageView		= view,
		.imageLayout	= layout
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.dstSet				= set,
		.dstBinding			= bindingNum,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,	
		.descriptorType		= vk::DescriptorType::eCombinedImageSampler,
		.pImageInfo			= &imageInfo
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void	Vulkan::WriteCombinedImageDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingNum, uint32_t subIndex, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout) {
	vk::DescriptorImageInfo imageInfo = {
		.sampler		= sampler,
		.imageView		= view,
		.imageLayout	= layout
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.dstSet				= set,
		.dstBinding			= bindingNum,
		.dstArrayElement	= subIndex,
		.descriptorCount	= 1,
		.descriptorType		= vk::DescriptorType::eCombinedImageSampler,
		.pImageInfo			= &imageInfo
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void	Vulkan::WriteSamplerDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingNum, vk::Sampler sampler) {
	vk::DescriptorImageInfo imageInfo = {
		.sampler		= sampler,
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.dstSet				= set,
		.dstBinding			= bindingNum,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= vk::DescriptorType::eSampler,
		.pImageInfo			= &imageInfo
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void	Vulkan::WriteStorageImageDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingNum, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout) {
	vk::DescriptorImageInfo imageInfo = {
		.sampler		= sampler,
		.imageView		= view,
		.imageLayout	= layout
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.dstSet				= set,
		.dstBinding			= bindingNum,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= vk::DescriptorType::eStorageImage,
		.pImageInfo			= &imageInfo
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void	Vulkan::WriteBufferDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingSlot, vk::DescriptorType bufferType, vk::Buffer buff, size_t offset, size_t range) {
	vk::DescriptorBufferInfo descriptorInfo = {
		.buffer = buff,
		.offset = offset,
		.range	= (range > 0 ? range : VK_WHOLE_SIZE)
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.dstSet				= set,
		.dstBinding			= bindingSlot,
		.descriptorCount	= 1,
		.descriptorType		= bufferType,
		.pBufferInfo		= &descriptorInfo
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void	Vulkan::WriteTLASDescriptor(vk::Device device, vk::DescriptorSet set, uint32_t bindingSlot, vk::AccelerationStructureKHR tlas) {
	vk::WriteDescriptorSetAccelerationStructureKHR descriptorInfo = {
		.accelerationStructureCount = 1,
		.pAccelerationStructures	= &tlas
	};

	vk::WriteDescriptorSet descriptorWrite = {
		.pNext				= &descriptorInfo,
		.dstSet				= set,
		.dstBinding			= bindingSlot,
		.descriptorCount	= 1,
		.descriptorType		= vk::DescriptorType::eAccelerationStructureKHR,
	};

	device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

vk::UniqueDescriptorSet Vulkan::CreateDescriptorSet(vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout  layout, uint32_t variableDescriptorCount) {
	vk::DescriptorSetAllocateInfo allocateInfo = {
		.descriptorPool		= pool,
		.descriptorSetCount = 1,
		.pSetLayouts		= &layout
	};
		
	vk::DescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorInfo;

	if (variableDescriptorCount > 0) {
		variableDescriptorInfo.setDescriptorSetCount(1).setPDescriptorCounts(&variableDescriptorCount);
		allocateInfo.setPNext((const void*)&variableDescriptorInfo);
	}
	return std::move(device.allocateDescriptorSetsUnique(allocateInfo)[0]);
}

vk::UniqueSemaphore Vulkan::CreateTimelineSemaphore(vk::Device device, uint64_t initialValue) {
	vk::SemaphoreTypeCreateInfo typeCreateInfo{
		.semaphoreType = vk::SemaphoreType::eTimeline,
		.initialValue = initialValue
	};
	vk::SemaphoreCreateInfo createInfo{
		.pNext = &typeCreateInfo,
	};
	return std::move(device.createSemaphoreUnique(createInfo));
}

vk::Result	Vulkan::TimelineSemaphoreHostWait(vk::Device device, vk::Semaphore semaphore, uint64_t waitVal, uint64_t waitTime) {
	vk::SemaphoreWaitInfo waitInfo{
		.semaphoreCount = 1,
		.pSemaphores	= &semaphore,
		.pValues		= &waitVal
	};
	return device.waitSemaphores(waitInfo, UINT64_MAX);
}

void	Vulkan::TimelineSemaphoreHostSignal(vk::Device device, vk::Semaphore semaphore, uint64_t signalVal) {
	vk::SemaphoreSignalInfo signalInfo{
		.semaphore	= semaphore,
		.value		= signalVal
	};
	device.signalSemaphore(signalInfo);
}

void	Vulkan::TimelineSemaphoreQueueWait(vk::Queue queue, vk::Semaphore semaphore, uint64_t waitVal, vk::PipelineStageFlags waitStage) {
	vk::TimelineSemaphoreSubmitInfo tlSubmit{
		.waitSemaphoreValueCount	= 1,
		.pWaitSemaphoreValues		= &waitVal
	};

	vk::SubmitInfo queueSubmit{
		.pNext					= &tlSubmit,
		.waitSemaphoreCount		= 1,
		.pWaitSemaphores		= &semaphore,
		.pWaitDstStageMask		= &waitStage
	};
	queue.submit(queueSubmit);
}

void		Vulkan::TimelineSemaphoreQueueSignal(vk::Queue queue, vk::Semaphore semaphore, uint64_t signalVal) {
	vk::TimelineSemaphoreSubmitInfo tlSubmit{
		.signalSemaphoreValueCount	= 1,
		.pSignalSemaphoreValues		= &signalVal
	};

	vk::SubmitInfo queueSubmit{
		.pNext					= &tlSubmit,
		.signalSemaphoreCount	= 1,
		.pSignalSemaphores		= &semaphore
	};

	queue.submit(queueSubmit);
}

/*Descriptor Buffer Writing*/
void Vulkan::WriteBufferDescriptor(vk::Device device,
	const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& props,
	void* descriptorBufferMemory,
	vk::DescriptorSetLayout layout,
	size_t layoutIndex,
	vk::DeviceAddress bufferAddress,
	size_t bufferSize
) {
	vk::DescriptorAddressInfoEXT address{
		.address	= bufferAddress,
		.range		= bufferSize
	};
	vk::DescriptorGetInfoEXT getInfo{
		.type = vk::DescriptorType::eUniformBuffer,
		.data = &address
	};
	
	vk::DeviceSize offset = device.getDescriptorSetLayoutBindingOffsetEXT(layout, layoutIndex);

	device.getDescriptorEXT(&getInfo, props.uniformBufferDescriptorSize, ((char*)descriptorBufferMemory) + offset);
}

size_t Vulkan::GetDescriptorSize(vk::DescriptorType type, const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& props) {
	switch (type) {
		case vk::DescriptorType::eSampler:					return props.samplerDescriptorSize;
		case vk::DescriptorType::eCombinedImageSampler:		return props.combinedImageSamplerDescriptorSize;
		case vk::DescriptorType::eSampledImage:				return props.sampledImageDescriptorSize;
		case vk::DescriptorType::eStorageImage:				return props.storageImageDescriptorSize;
		case vk::DescriptorType::eUniformTexelBuffer:		return props.uniformTexelBufferDescriptorSize;
		case vk::DescriptorType::eStorageTexelBuffer:		return props.storageTexelBufferDescriptorSize;
		case vk::DescriptorType::eUniformBuffer:			return props.uniformBufferDescriptorSize;
		case vk::DescriptorType::eStorageBuffer:			return props.storageBufferDescriptorSize;
		case vk::DescriptorType::eUniformBufferDynamic:		return props.uniformBufferDescriptorSize;//??
		case vk::DescriptorType::eStorageBufferDynamic:		return props.storageBufferDescriptorSize;//??
		case vk::DescriptorType::eInputAttachment:			return props.inputAttachmentDescriptorSize;		
		case vk::DescriptorType::eAccelerationStructureKHR: return props.accelerationStructureDescriptorSize;
		default: return 0;
	}
};

void  Vulkan::UploadTextureData(vk::CommandBuffer  buffer, vk::Buffer tempBuffer, vk::Image image, vk::ImageLayout currentLyout, vk::ImageLayout endLayout, vk::BufferImageCopy copyInfo) {
	ImageTransitionBarrier(buffer, image, 
		currentLyout, 
		vk::ImageLayout::eTransferDstOptimal, 
		copyInfo.imageSubresource.aspectMask, 
		vk::PipelineStageFlagBits2::eHost, 
		vk::PipelineStageFlagBits2::eTransfer, 
		0, 1);

	buffer.copyBufferToImage(tempBuffer, image, vk::ImageLayout::eTransferDstOptimal, copyInfo);

	ImageTransitionBarrier(buffer, image, 
		vk::ImageLayout::eTransferDstOptimal, 
		endLayout, 
		copyInfo.imageSubresource.aspectMask, 
		vk::PipelineStageFlagBits2::eTransfer, 
		vk::PipelineStageFlagBits2::eAllCommands,
		0, 1);
}

bool  Vulkan::FormatIsDepth(vk::Format format) {
	switch (format) {
	case vk::Format::eD16Unorm:
	case vk::Format::eD32Sfloat:
	case vk::Format::eX8D24UnormPack32:
		return true;
	}
	return false;
}

bool  Vulkan::FormatIsDepthStencil(vk::Format format) {
	switch (format) {
	case vk::Format::eD16UnormS8Uint:
	case vk::Format::eD24UnormS8Uint:
	case vk::Format::eD32SfloatS8Uint:
		return true;
	}
	return false;
}