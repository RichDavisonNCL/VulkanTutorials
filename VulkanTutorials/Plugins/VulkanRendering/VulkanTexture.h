/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/TextureBase.h"

namespace NCL::Rendering {
	class VulkanRenderer;
	class VulkanTexture : public TextureBase	{
		friend class VulkanRenderer;
	public:

		static std::shared_ptr<VulkanTexture> VulkanCubemapPtrFromFilename(
			const std::string& negativeXFile, const std::string& positiveXFile,
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,
			const std::string& debugName = "CubeMap");

		static VulkanTexture* VulkanCubemapFromFilename(
			const std::string& negativeXFile, const std::string& positiveXFile, 
			const std::string& negativeYFile, const std::string& positiveYFile,
			const std::string& negativeZFile, const std::string& positiveZFile,
			const std::string& debugName = "CubeMap");

		static TextureBase* TextureFromFilenameLoader(const std::string& name);

		static std::shared_ptr<VulkanTexture> TexturePtrFromFilename(const std::string& name);
		static std::shared_ptr<VulkanTexture> GenerateDepthTexturePtr(int width, int height, const std::string& debugName = "DefaultDepth", bool hasStencil = true, bool mips = false);
		static std::shared_ptr<VulkanTexture> GenerateColourTexturePtr(int width, int height, const std::string& debugName = "DefaultColour", bool isFloat = false, bool mips = false);


		static VulkanTexture* TextureFromFilename(const std::string& name);
		static VulkanTexture* GenerateDepthTexture(int width, int height, const std::string& debugName = "DefaultDepth", bool hasStencil = true, bool mips = false);
		static VulkanTexture* GenerateColourTexture(int width, int height, const std::string& debugName = "DefaultColour", bool isFloat = false, bool mips = false);

		vk::ImageView GetDefaultView() const {
			return defaultView.get();
		}

		vk::Format GetFormat() const {
			return format;
		}

		vk::Image GetImage() const {
			return image.get();
		}

		~VulkanTexture();

	protected:

		VulkanTexture();
		void GenerateMipMaps(vk::CommandBuffer  buffer, vk::ImageLayout endLayout, vk::PipelineStageFlags endFlags);

		static void	InitTextureDeviceMemory(VulkanTexture& img);
		static VulkanTexture* GenerateTextureInternal(int width, int height, int mipcount, bool isCube, const std::string& debugName, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageUsageFlags usage, vk::ImageLayout outLayout, vk::PipelineStageFlags pipeType);

		static VulkanTexture* GenerateTextureFromDataInternal(int width, int height, int channelCount, bool isCube, std::vector<char*>dataSrcs, const std::string& debugName);


		vk::UniqueImageView  GenerateDefaultView(vk::ImageAspectFlags type);

		static int CalculateMipCount(int width, int height);

		vk::UniqueImageView		defaultView;
		vk::UniqueImage			image;
		//vk::ImageLayout			layout;

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