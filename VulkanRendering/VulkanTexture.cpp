/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanTexture.h"
#include "Vulkanrenderer.h"
#include "TextureLoader.h"
#include "VulkanUtils.h"
#include "VulkanBuffer.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

VulkanTexture::VulkanTexture() {
}

VulkanTexture::~VulkanTexture() {
	if (m_image && m_memManager) {
		m_memManager->DiscardImage(m_image);
	}
}

void VulkanTexture::GenerateMipMaps(vk::CommandBuffer  buffer, vk::ImageLayout startLayout, vk::ImageLayout endLayout, vk::PipelineStageFlags2 endFlags) {
	for (int layer = 0; layer < m_layerCount; ++layer) {	
		ImageTransitionBarrier(buffer, m_image, 
			startLayout, vk::ImageLayout::eTransferSrcOptimal,
			m_aspectType, 
			vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eTransfer, 
			0, 1, layer, 1);

		vk::ImageBlit blitData;
		blitData.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setMipLevel(0)
			.setBaseArrayLayer(layer)
			.setLayerCount(1);

		blitData.srcOffsets[0] = vk::Offset3D(0, 0, 0);
		blitData.srcOffsets[1] = vk::Offset3D(dimensions.x, dimensions.y, 1);

		blitData.dstOffsets[0] = vk::Offset3D(0, 0, 0);
		blitData.dstOffsets[1] = vk::Offset3D(dimensions.x, dimensions.y, 1);

		for (uint32_t mip = 1; mip < m_mipCount; ++mip) {
			blitData.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(mip)
				.setBaseArrayLayer(layer)
				.setLayerCount(1);

			blitData.dstOffsets[1].x = std::max(dimensions.x >> mip, 1u);
			blitData.dstOffsets[1].y = std::max(dimensions.y >> mip, 1u);

			//Prepare the new mip level to receive the blitted data
			ImageTransitionBarrier(buffer, m_image, 
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 
				m_aspectType, 
				vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer,
				mip, 1, layer, 1);
			
			buffer.blitImage(m_image, vk::ImageLayout::eTransferSrcOptimal, 
							 m_image, vk::ImageLayout::eTransferDstOptimal, 
							 blitData, vk::Filter::eLinear);
			//The new mip is then transitioned again to be able to act as a source
			//for the next level 
			ImageTransitionBarrier(buffer, m_image,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
				m_aspectType,
				vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer,
				mip, 1, layer, 1);

			blitData.srcOffsets		= blitData.dstOffsets;
			blitData.srcSubresource = blitData.dstSubresource;
		}
		//Now that this layer's mipchain is complete, transition the whole lot to final layout
		ImageTransitionBarrier(buffer, m_image, 
				vk::ImageLayout::eTransferSrcOptimal, endLayout,
				m_aspectType, 
				vk::PipelineStageFlagBits2::eTransfer, endFlags,
				0, m_mipCount, layer, 1);
	}
}
