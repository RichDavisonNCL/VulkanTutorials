/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering::Vulkan {
	/*
	DescriptorBufferWriter: A helper class for writing descriptors to a 
	descriptor buffer. We MUST call Finish to ensure that the writes to the
	buffer are synchronised, in case the buffer is not persistently mapped.
	*/
	class DescriptorBufferWriter {
	public:
		DescriptorBufferWriter(vk::Device inDevice, vk::DescriptorSetLayout inLayout, VulkanBuffer& inBuffer, vk::PhysicalDeviceDescriptorBufferPropertiesEXT& inProps)
		 : m_destBuffer(inBuffer)
		 , m_props(inProps)	{
			m_device = inDevice;
			m_layout = inLayout;
			m_descriptorBufferMemory = m_destBuffer.Map();
		}

		~DescriptorBufferWriter() 
		{
			if (m_descriptorBufferMemory) {
				Finish();
			}
		}

		DescriptorBufferWriter& WriteBuffer(uint32_t binding, vk::DescriptorType type, const VulkanBuffer& buffer, uint32_t arrayIndex = 0) 
		{
			vk::DescriptorAddressInfoEXT descriptorAddress = {
				.address	= buffer.deviceAddress,
				.range		= buffer.size
			};

			vk::DescriptorGetInfoEXT getInfo = {
				.type = type,
				.data = &descriptorAddress
			};

			vk::DeviceSize		offset = m_device.getDescriptorSetLayoutBindingOffsetEXT(m_layout, binding);

			size_t descriptorSize = Vulkan::GetDescriptorSize(type, m_props);

			m_device.getDescriptorEXT(&getInfo, descriptorSize, ((char*)m_descriptorBufferMemory) + offset);

			return *this;
		}

		DescriptorBufferWriter& WriteCombinedImageSampler(uint32_t binding,vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout, uint32_t arrayIndex = 0) 
		{
			vk::DescriptorImageInfo imageInfo = {
				.sampler		= sampler,
				.imageView		= view,
				.imageLayout	= layout
			};

			vk::DescriptorType type = vk::DescriptorType::eCombinedImageSampler;

			vk::DescriptorGetInfoEXT getInfo = {
				.type = type,
				.data = &imageInfo
			};

			vk::DeviceSize		offset = m_device.getDescriptorSetLayoutBindingOffsetEXT(m_layout, binding);

			size_t descriptorSize = Vulkan::GetDescriptorSize(type, m_props);

			m_device.getDescriptorEXT(&getInfo, descriptorSize, ((char*)m_descriptorBufferMemory) + offset);

			return *this;
		}

		void Finish() 
		{
			m_destBuffer.Unmap();
			m_descriptorBufferMemory = nullptr;
		}

	protected:
		vk::Device				m_device;
		VulkanBuffer&			m_destBuffer;
		void*					m_descriptorBufferMemory;
		vk::DescriptorSetLayout m_layout;
		vk::PhysicalDeviceDescriptorBufferPropertiesEXT& m_props;
	};
};