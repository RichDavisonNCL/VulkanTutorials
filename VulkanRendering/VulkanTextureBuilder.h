/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "SmartTypes.h"
#include "VulkanBuffer.h"
#include "VulkanMemoryManager.h"

namespace NCL::Rendering::Vulkan {
	class VulkanMemoryManager;

	class TextureBuilder	{
	public:
		TextureBuilder(vk::Device device, VulkanMemoryManager& memManager);
		~TextureBuilder() {}

		TextureBuilder& WithFormat(vk::Format format);
		TextureBuilder& WithLayout(vk::ImageLayout layout);
		TextureBuilder& WithAspects(vk::ImageAspectFlags aspects);
		TextureBuilder& WithUsages(vk::ImageUsageFlags usages);
		TextureBuilder& WithPipeFlags(vk::PipelineStageFlags2 flags);

		TextureBuilder& WithCommandBuffer(vk::CommandBuffer buffer);

		TextureBuilder& WithMips(bool state);
		TextureBuilder& WithDimension(uint32_t width, uint32_t height, uint32_t depth = 1);
		TextureBuilder& WithLayerCount(uint32_t layers);

		//Builds an empty texture
		UniqueVulkanTexture Build(const std::string& debugName = "");

		//Builds a specifically sized texture using provided data is input
		UniqueVulkanTexture BuildFromData(void* dataSrc, size_t byteCount, const std::string& debugName = "");

		//Builds a texture loaded from file
		UniqueVulkanTexture BuildFromFile(const std::string& filename);

		//Builds an empty cubemap
		UniqueVulkanTexture BuildCubemap(const std::string& debugName = "");

		//Builds a cubemap from file set
		UniqueVulkanTexture BuildCubemapFromFile(
			const std::string& negativeXFile, const std::string& positiveXFile,
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,	
			const std::string& debugName = "");

	protected:
		struct TextureJob {
			vk::Image		image;
			vk::ImageLayout endLayout;
			vk::ImageAspectFlags aspect;
			size_t			faceByteCount = 0;

			NCL::Maths::Vector3ui		dimensions;

			uint32_t faceCount		= 0;

			char* dataSrcs[6]		= { nullptr };
		};

		void FinaliseTexture(const std::string& debugName, vk::CommandBuffer& usingBuffer, TextureJob& job, UniqueVulkanTexture& t);

		UniqueVulkanTexture	GenerateTexture(vk::CommandBuffer m_cmdBuffer, Maths::Vector3ui dimensions, bool isCube, const std::string& debugName);

		void UploadTextureData(vk::CommandBuffer buffer, TextureJob& job);

		NCL::Maths::Vector3ui	m_requestedSize;
		uint32_t				m_layerCount;
		bool					m_generateMips;

		vk::Format				m_format;
		vk::ImageLayout			m_layout;
		vk::ImageAspectFlags	m_aspects;
		vk::ImageUsageFlags		m_usages;
		vk::PipelineStageFlags2	m_pipeFlags;

		vk::Device				m_sourceDevice;

		VulkanMemoryManager*	m_memManager;

		vk::CommandBuffer		m_cmdBuffer;
	};
}