set(PROJECT_NAME VulkanRayTracing)

################################################################################
# Source groups
################################################################################
set(Header_Files
	"TestRayTrace.h"
	"TestGLTFRayTrace.h"
	"VulkanBVHBuilder.h"
	"VulkanRTShader.h" 
	"VulkanRayTracingPipelineBuilder.h"	
	"VulkanShaderBindingTableBuilder.h"
	"TestRayTracedSkinning.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
	"TestRayTrace.cpp"
	"TestGLTFRayTrace.cpp"
	"VulkanBVHBuilder.cpp"
	"VulkanRTShader.cpp"   
	"VulkanRayTracingPipelineBuilder.cpp"
	"VulkanShaderBindingTableBuilder.cpp"	
	"TestRayTracedSkinning.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <vulkan/vulkan.hpp>
	<vma/vk_mem_alloc.h>
    <memory>
    <unordered_set>
    <map>
    <vector>
    <set>
    <string>
    <fstream>
    <iostream>
    <string>
    <iosfwd>
	
	<Matrix2.h>
    <Matrix3.h>
    <Matrix4.h>

    <Vector2.h>
    <Vector3.h>
    <Vector4.h>

    <Vector2i.h>
    <Vector3i.h>
    <Vector4i.h>
	
	<Quaternion.h>
	
	<Camera.h>
    <TextureLoader.h>
	
	<SmartTypes.h>
    <VulkanRenderer.h>
    <VulkanShader.h>
    <VulkanShaderBuilder.h>
    <VulkanTexture.h>
    <VulkanMesh.h>
    <VulkanPipelineBuilder.h>
    <VulkanDynamicRenderBuilder.h>

    <VulkanDescriptorSetLayoutBuilder.h>
    <VulkanRenderPassBuilder.h>
    <VulkanCompute.h>
    <VulkanComputePipelineBuilder.h>

    <VulkanBufferBuilder.h>
)

add_compile_definitions(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
add_compile_definitions(VK_NO_PROTOTYPES)

################################################################################
# Compile and link options
################################################################################
target_include_directories (${PROJECT_NAME} 
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)	

target_include_directories (${PROJECT_NAME} 
    PUBLIC ${Vulkan_INCLUDE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/NCLCoreClasses
	PUBLIC ${CMAKE_SOURCE_DIR}/VulkanRendering
)	
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
endif()