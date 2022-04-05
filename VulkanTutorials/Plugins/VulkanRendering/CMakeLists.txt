set(PROJECT_NAME VulkanRendering)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "Precompiled.h"
    "Vulkan.h"
    "VulkanBuffers.h"
    "VulkanBVHBuilder.h"
    "VulkanCompute.h"
    "VulkanComputePipelineBuilder.h"
    "VulkanDescriptorSetLayoutBuilder.h"
    "VulkanDynamicRenderBuilder.h"
    "VulkanMesh.h"
    "VulkanPipeline.h"
    "VulkanPipelineBuilder.h"
    "VulkanRenderer.h"
    "VulkanRenderPassBuilder.h"
    "VulkanRTShader.h"
    "VulkanShader.h"
    "VulkanShaderBuilder.h"
    "VulkanTexture.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "Precompiled.cpp"
    "Vulkan.cpp"
    "VulkanBuffers.cpp"
    "VulkanBVHBuilder.cpp"
    "VulkanCompute.cpp"
    "VulkanComputePipelineBuilder.cpp"
    "VulkanDescriptorSetLayoutBuilder.cpp"
    "VulkanDynamicRenderBuilder.cpp"
    "VulkanMesh.cpp"
    "VulkanPipelineBuilder.cpp"
    "VulkanRenderer.cpp"
    "VulkanRenderPassBuilder.cpp"
    "VulkanRTShader.cpp"
    "VulkanShader.cpp"
    "VulkanShaderBuilder.cpp"
    "VulkanTexture.cpp"
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

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Precompiled.h>"
)

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE VulkanRendering)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "VK_PROTOTYPES"
    ">"
    "NOMINMAX;"
    "_MBCS"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy
        >
        /permissive-;
        /std:c++latest;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
        >
    )
endif()

target_include_directories (${PROJECT_NAME} 
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)	

target_include_directories (${PROJECT_NAME} 
    PUBLIC ${Vulkan_INCLUDE_DIR}
)	

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
    ">"
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

if(MSVC)
target_link_libraries(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/lib/VkLayer_utils.lib")
endif()