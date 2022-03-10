/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Common/RendererBase.h"
#include "../../Common/Maths.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "vulkan/vulkan.hpp"

#include "VulkanPipeline.h"

#include <vector>
#include <string>

using std::string;

namespace NCL::Rendering {
	class VulkanMesh;
	class VulkanShader;
	class VulkanCompute;
	class VulkanTexture;
	struct VulkanBuffer;
	struct BufferedData;


	struct VulkanInitInfo {
		std::vector<const char*> extensions;
		std::vector<const char*> layers;

		int minorVersion = 1;
		int majorVersion = 1;

		VulkanInitInfo(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
			this->extensions	= extensions;
			this->layers		= layers;
		}
		VulkanInitInfo() {

		}
	};

	class VulkanRenderer : public RendererBase {
		friend class VulkanMesh;
		friend class VulkanTexture;
		friend class VulkanPipelineBuilder;
		friend class VulkanShaderBuilder;
		friend class VulkanDescriptorSetLayoutBuilder;
		friend class VulkanRenderPassBuilder;
		friend class VulkanBVHBuilder;
	public:
		VulkanRenderer(Window& window, VulkanInitInfo info = VulkanInitInfo());
		~VulkanRenderer();

		//vk::ClearColorValue ClearColour(const Vector4& v) {
		//	return vk::ClearColorValue(std::array<float, 4>{v.x, v.y, v.z, v.w});
		//}

		vk::ClearColorValue ClearColour(float r, float g, float b, float a = 1.0f) {
			return vk::ClearColorValue(std::array<float, 4>{r, g, b, a});
		}

	protected:
		void OnWindowResize(int w, int h)	override;
		void BeginFrame()		override;
		void EndFrame()			override;
		void SwapBuffers()		override;

		virtual void	CompleteResize();
		virtual void	InitDefaultRenderPass();
		virtual void	InitDefaultDescriptorPool();

		void SubmitDrawCall(const VulkanMesh* m, vk::CommandBuffer  to);
		void SubmitDrawCallLayer(const VulkanMesh* m, unsigned int layer, vk::CommandBuffer  to);

		void DispatchCompute(vk::CommandBuffer  to, unsigned int xCount, unsigned int yCount = 0, unsigned int zCount = 0);

		vk::DescriptorSet		BuildDescriptorSet(vk::DescriptorSetLayout  layout);
		vk::UniqueDescriptorSet BuildUniqueDescriptorSet(vk::DescriptorSetLayout  layout);

		void	UpdateUniformBufferDescriptor(vk::DescriptorSet set, const VulkanBuffer& data, int bindingSlot);
		void	UpdateImageDescriptor(vk::DescriptorSet set, int bindingNum, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal);

		void	ImageTransitionBarrier(vk::CommandBuffer  buffer, vk::Image i, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage, int mipLevel = 0, int layer = 0 );
		void	ImageTransitionBarrier(vk::CommandBuffer  buffer, const VulkanTexture* t, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage, int mipLevel = 0, int layer = 0);
	
		vk::CommandBuffer	BeginComputeCmdBuffer(const std::string& debugName = "");
		vk::CommandBuffer	BeginCmdBuffer(const std::string& debugName = "");

		void				SubmitCmdBufferWait(vk::CommandBuffer buffer, bool andFree = false);
		void				SubmitCmdBuffer(vk::CommandBuffer  buffer, bool andFree = false);
		vk::Fence 			SubmitCmdBufferFence(vk::CommandBuffer  buffer, bool andFree = false);

		void				DestroyCmdBuffer(vk::CommandBuffer  buffer);

		VulkanBuffer CreateBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal);
		void		 DestroyBuffer(VulkanBuffer& uniform);
		void		 UploadBufferData(VulkanBuffer& uniform, void* data, int dataSize);

		void		BeginDefaultRenderPass(vk::CommandBuffer  cmds);

		void		BeginDefaultRendering(vk::CommandBuffer  cmds);
		void		EndRendering(vk::CommandBuffer  cmds);

		vk::Device GetDevice() const {
			return device;
		}

		bool	MemoryTypeFromPhysicalDeviceProps(vk::MemoryPropertyFlags requirements, uint32_t type, uint32_t& index);

		bool EnableRayTracing();

		void TransitionSwapchainForRendering(vk::CommandBuffer buffer);
		void TransitionSwapchainForPresenting(vk::CommandBuffer buffer);

		const vk::PhysicalDeviceRayTracingPipelinePropertiesKHR&  GetRayTracingPipelineProperties() const { return rayPipelineProperties; }
		const vk::PhysicalDeviceAccelerationStructureFeaturesKHR& GetRayTracingAccelerationStructureProperties() const { return rayAccelFeatures; }

	protected:		
		struct SwapChain {
			vk::Image			image;
			vk::ImageView		view;
		};

		vk::CommandBuffer		defaultCmdBuffer;
		vk::PipelineCache		pipelineCache;
		vk::RenderPass			defaultRenderPass;
		vk::RenderPassBeginInfo defaultBeginInfo;
		vk::Device				device;		//Device handle	
		
		vk::ClearValue			defaultClearValues[2];
		vk::Viewport			defaultViewport;
		vk::Rect2D				defaultScissor;	
		vk::Rect2D				defaultScreenRect;	

		std::vector<SwapChain*> swapChainList;	
		
		uint32_t				currentSwap;

		vk::Framebuffer*		frameBuffers;		
		
		vk::DescriptorPool		defaultDescriptorPool;	//descriptor sets come from here!
		vk::CommandPool			commandPool;			//Source Command Buffers from here
		vk::CommandPool			computeCommandPool;		//Source Command Buffers from here

	//private:
		void	InitCommandPools();
		void	PresentScreenImage();
		bool	InitInstance(int major, int minor);
		bool	InitGPUDevice();
		bool	InitSurface();
		int		InitBufferChain(vk::CommandBuffer  cmdBuffer);

		bool	InitDeviceQueues();
		bool	CreateDefaultFrameBuffers();

		vk::DescriptorSetLayout nullLayout;
		vk::SurfaceKHR		surface;
		vk::Format			surfaceFormat;
		vk::ColorSpaceKHR	surfaceSpace;

		uint32_t			numFrameBuffers;
		std::shared_ptr<VulkanTexture> 	depthBuffer;

		vk::SwapchainKHR	swapChain;

		vk::Instance		instance;	//API Instance
		vk::PhysicalDevice	gpu;		//GPU in use

		vk::PhysicalDeviceProperties		deviceProperties;
		vk::PhysicalDeviceMemoryProperties	deviceMemoryProperties;



		vk::Queue			deviceQueue;
		uint32_t			computeQueueIndex;
		uint32_t			gfxQueueIndex;
		uint32_t			gfxPresentIndex;

		std::vector<const char*> extensionList;
		std::vector<const char*> layerList;

		/*
		* RayTracing Stuff!
		*/
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR	rayPipelineProperties;
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR	rayAccelFeatures;
	};
}