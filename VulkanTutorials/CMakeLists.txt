set(PROJECT_NAME VulkanTutorials)

################################################################################
# Source groups
################################################################################

file(GLOB_RECURSE Header_Files CONFIGURE_DEPENDS *.h)
source_group("Header Files" FILES ${Header_Files})

file(GLOB_RECURSE Source_Files CONFIGURE_DEPENDS *.cpp)
source_group("Source Files" FILES ${Source_Files})

file(GLOB_RECURSE ALL_SHADERS CONFIGURE_DEPENDS 
	${ASSET_ROOT}Shaders/*.vert 
	${ASSET_ROOT}Shaders/*.frag 
	${ASSET_ROOT}Shaders/*.comp 
	${ASSET_ROOT}Shaders/*.geom 
	${ASSET_ROOT}Shaders/*.tesc 
	${ASSET_ROOT}Shaders/*.tese 
	${ASSET_ROOT}Shaders/*.rgen 
	${ASSET_ROOT}Shaders/*.rint 
	${ASSET_ROOT}Shaders/*.rahit 
	${ASSET_ROOT}Shaders/*.rchit 
	${ASSET_ROOT}Shaders/*.rmiss 
	${ASSET_ROOT}Shaders/*.rcall 
	${ASSET_ROOT}Shaders/*.task 
	${ASSET_ROOT}Shaders/*.mesh)
	
source_group("Shader Files" FILES ${ALL_SHADERS})

set(ALL_FILES
    ${Graphics_Techniques}
    ${Vulkan_API}

    ${Header_Files}
    ${Source_Files}

	${ALL_SHADERS}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <vulkan/vulkan.hpp>
    <memory>    
	<set>
    <unordered_set>
	<map>
    <vector>
    <string>
    <fstream>
    <iostream>
    <string>
    <iosfwd>
    <chrono>

    <Quaternion.h>
    <Vector.h>
    <Matrix.h>

    <Camera.h>
    <GameTimer.h>

    <TextureLoader.h>

    <SmartTypes.h>
    <VulkanRenderer.h>
    <VulkanShader.h>
    <VulkanShaderBuilder.h>
    <VulkanTexture.h>
    <VulkanMesh.h>
    <VulkanPipelineBuilder.h>
    <VulkanDynamicRenderBuilder.h>

    <VulkanTextureBuilder.h>

    <VulkanDescriptorSetLayoutBuilder.h>
    <VulkanRenderPassBuilder.h>
    <VulkanCompute.h>
    <VulkanComputePipelineBuilder.h>
	<VulkanBufferBuilder.h>

    <VulkanDescriptorSetWriter.h>
    <VulkanDescriptorSetBinder.h>   
    <VulkanDescriptorBufferWriter.h>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
endif()

################################################################################
# Dependencies
################################################################################
target_include_directories (${PROJECT_NAME} 
    PUBLIC ${Vulkan_INCLUDE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/NCLCoreClasses
    PUBLIC ${CMAKE_SOURCE_DIR}/VulkanRendering
    PUBLIC ${CMAKE_SOURCE_DIR}/GLTFLoader
)	

foreach (file ${ALL_SHADERS})
    get_filename_component(file_name ${file} NAME)
	get_filename_component(file_ext ${file} EXT)
	
	message("Adding custom command to ${file}")
	get_filename_component(file_dir ${file} ABSOLUTE)
	set(SPIRV_OUTPUT ${file_name}.spv)
	set(SPIRV_ABS_INPUT ${file_dir})
	set(SPIRV_ABS_OUTPUT ${file_dir}.spv)
	
	add_custom_command(
		OUTPUT ${SPIRV_ABS_OUTPUT}
		
		COMMENT "Compiling GLSL shader:"
		COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V  --target-env vulkan1.2 ${SPIRV_ABS_INPUT} -o ${SPIRV_ABS_OUTPUT}
		DEPENDS ${file}
		VERBATIM
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})   
endforeach()

add_custom_target(
    Shaders
    DEPENDS ON ${SPIRV_BINARY_FILES}
)

set(PROJECT_DEPENDENCIES
    NCLCoreClasses
    VulkanRendering
    GLTFLoader
)

if(ADD_RAY_TRACING)
	target_include_directories (${PROJECT_NAME} 
		PUBLIC ${CMAKE_SOURCE_DIR}/VulkanRayTracing
	)	

	set(PROJECT_DEPENDENCIES
		${PROJECT_DEPENDENCIES}
		VulkanRayTracing
	)
endif()

add_dependencies(${PROJECT_NAME}
    ${PROJECT_DEPENDENCIES}
    Shaders
)

add_compile_definitions(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
add_compile_definitions(VK_NO_PROTOTYPES)
add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${PROJECT_DEPENDENCIES}
)