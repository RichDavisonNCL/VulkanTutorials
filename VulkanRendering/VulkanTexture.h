/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/Texture.h"
#include "VulkanTextureBuilder.h"
#include "SmartTypes.h"

namespace NCL::Rendering::Vulkan {
	class VulkanRenderer;
	class VulkanMemoryManager;

	class VulkanTexture : public Texture	{
		friend class VulkanRenderer;
		friend class TextureBuilder;
	public:
		~VulkanTexture();

		vk::ImageView GetDefaultView() const {
			return *m_defaultView;
		}

		vk::Format GetFormat() const {
			return m_format;
		}

		vk::Image GetImage() const {
			return m_image;
		}

		//Allows us to pass a texture as vk type to various functions
		operator vk::Image() const {
			return m_image;
		}
		operator vk::ImageView() const {
			return *m_defaultView;
		}		
		operator vk::Format() const {
			return m_format;
		}

		void GenerateMipMaps(	vk::CommandBuffer  buffer, 
								vk::ImageLayout startLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
								vk::ImageLayout endLayout	= vk::ImageLayout::eShaderReadOnlyOptimal, 
								vk::PipelineStageFlags2 endFlags = vk::PipelineStageFlagBits2::eFragmentShader);

		template <typename T, uint32_t n>
		static constexpr size_t GetMaxMips(const VectorTemplate<T, n>& texDims) {
			T m = Vector::GetMaxElement(texDims);
			return (size_t)std::floor(log2((float(m)))) + 1;
		}

	protected:
		VulkanTexture();

		vk::UniqueImageView		m_defaultView;
		vk::Image				m_image; //Don't use 'Unique', must use the memory manager
		vk::Format				m_format = vk::Format::eUndefined;
		vk::ImageAspectFlags	m_aspectType;

		vk::UniqueSemaphore		m_workSemaphore;

		VulkanMemoryManager*	m_memManager = nullptr;

		uint32_t m_mipCount		= 0;
		uint32_t m_layerCount	= 0;
	};
}