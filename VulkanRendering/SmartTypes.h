/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace NCL::Rendering::Vulkan {
	class VulkanMesh;
	class VulkanShader;
	class VulkanCompute;
	class VulkanTexture;
	class VulkanBuffer;
	class VulkanShaderModule;

	using UniqueVulkanMesh		= std::unique_ptr<VulkanMesh>;
	using SharedVulkanMesh		= std::shared_ptr<VulkanMesh>;

	using UniqueVulkanShader	= std::unique_ptr<VulkanShader>;
	using SharedVulkanShader	= std::shared_ptr<VulkanShader>;

	using UniqueVulkanCompute	= std::unique_ptr<VulkanCompute>;
	using SharedVulkanCompute	= std::shared_ptr<VulkanCompute>;

	using UniqueVulkanTexture	= std::unique_ptr<VulkanTexture>;
	using SharedVulkanTexture	= std::shared_ptr<VulkanTexture>;

	using UniqueVulkanBuffer	= std::unique_ptr<VulkanBuffer>;
	using SharedVulkanBuffer	= std::shared_ptr<VulkanBuffer>;

	using UniqueVulkanShaderModule = std::unique_ptr<VulkanShaderModule>;
	using SharedVulkanShaderModule = std::shared_ptr<VulkanShaderModule>;
}