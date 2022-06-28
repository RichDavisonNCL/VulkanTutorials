/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/TextureBase.h"
#include "SmartTypes.h"

namespace NCL::Rendering {
	class VulkanRenderer;

	class VulkanTexture : public TextureBase	{
		friend class VulkanRenderer;
	public:

		static UniqueVulkanTexture VulkanCubemapFromFiles(
			const std::string& negativeXFile, const std::string& positiveXFile, 
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,
			const std::string& debugName = "CubeMap");

		static TextureBase* TextureFromFilenameLoader(const std::string& name);//Compatability function with old course modules

		static UniqueVulkanTexture TextureFromFile(const std::string& name);
		static UniqueVulkanTexture CreateDepthTexture(uint32_t width, uint32_t height, const std::string& debugName = "DefaultDepth", bool hasStencil = true, bool mips = false);
		static UniqueVulkanTexture CreateColourTexture(uint32_t width, uint32_t height, const std::string& debugName = "DefaultColour", bool isFloat = false, bool mips = false);

		vk::ImageView GetDefaultView() const {
			return *defaultView;
		}

		vk::Format GetFormat() const {
			return format;
		}

		vk::Image GetImage() const {
			return *image;
		}

		~VulkanTexture();

	protected:

		VulkanTexture();
		void GenerateMipMaps(vk::CommandBuffer  buffer, vk::ImageLayout endLayout, vk::PipelineStageFlags endFlags);

		static void	InitTextureDeviceMemory(VulkanTexture& img);
		static VulkanTexture* GenerateTextureInternal(uint32_t width, uint32_t height, uint32_t mipcount, bool isCube, const std::string& debugName, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageUsageFlags usage, vk::ImageLayout outLayout, vk::PipelineStageFlags pipeType);

		static VulkanTexture* GenerateTextureFromDataInternal(uint32_t width, uint32_t height, uint32_t channelCount, bool isCube, std::vector<char*>dataSrcs, const std::string& debugName);

		vk::UniqueImageView  GenerateDefaultView(vk::ImageAspectFlags type);

		static int CalculateMipCount(uint32_t width, uint32_t height);

		vk::UniqueImageView		defaultView;
		vk::UniqueImage			image;

		vk::DeviceMemory		deviceMem;

		vk::Format				format;

		vk::MemoryAllocateInfo	allocInfo;
		vk::ImageCreateInfo		createInfo;
		vk::ImageAspectFlags	aspectType;

		int width;
		int height;
		int mipCount;
		int layerCount;

		static void SetRenderer(VulkanRenderer* r) {
			vkRenderer = r;
		}	

		static VulkanRenderer* vkRenderer;
	};
}