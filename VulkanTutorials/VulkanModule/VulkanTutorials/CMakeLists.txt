set(PROJECT_NAME VulkanTutorials)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "BasicComputeUsage.h"
    "BasicDescriptorRenderer.h"
    "BasicGeometryRenderer.h"
    "BasicMultiPassRenderer.h"
    "BasicMultiPipelineRenderer.h"
    "BasicPushConstantRenderer.h"
    "BasicSkinningExample.h"
    "BasicTexturingRenderer.h"
    "BasicUniformBufferRenderer.h"
    "CubeMapRenderer.h"
    "DeferredExample.h"
    "GLTFExample.h"
    "LightingExample.h"
    "MultiViewportExample.h"
    "PostProcessingExample.h"
    "Precompiled.h"
    "PrerecordedCmdListRenderer.h"
    "resource.h"
    "ShadowmappingExample.h"
    "TestRayTrace.h"
    "VulkanTutorialRenderer.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Shaders
    "../../Assets/Shaders/VK/BasicCompute.comp"
    "../../Assets/Shaders/VK/BasicCompute.frag"
    "../../Assets/Shaders/VK/BasicCompute.vert"
    "../../Assets/Shaders/VK/BasicDescriptor.frag"
    "../../Assets/Shaders/VK/BasicDescriptor.vert"
    "../../Assets/Shaders/VK/BasicGeometry.frag"
    "../../Assets/Shaders/VK/BasicGeometry.vert"
    "../../Assets/Shaders/VK/BasicPushConstant.frag"
    "../../Assets/Shaders/VK/BasicPushConstant.vert"
    "../../Assets/Shaders/VK/BasicSkinning.vert"
    "../../Assets/Shaders/VK/BasicTexturing.frag"
    "../../Assets/Shaders/VK/BasicTexturing.vert"
    "../../Assets/Shaders/VK/BasicUniformBuffer.frag"
    "../../Assets/Shaders/VK/BasicUniformBuffer.vert"
    "../../Assets/Shaders/VK/cubemapObject.frag"
    "../../Assets/Shaders/VK/cubemapObject.vert"
    "../../Assets/Shaders/VK/Lighting.frag"
    "../../Assets/Shaders/VK/Lighting.vert"
    "../../Assets/Shaders/VK/PostProcess.frag"
    "../../Assets/Shaders/VK/PostProcess.vert"
    "../../Assets/Shaders/VK/SimpleVertexTransform.vert"
    "../../Assets/Shaders/VK/SingleTexture.frag"
    "../../Assets/Shaders/VK/skybox.frag"
    "../../Assets/Shaders/VK/skybox.vert"
    "../../Assets/Shaders/VK/SolidColour.frag"
    "../../Assets/Shaders/VK/tri.frag"
    "../../Assets/Shaders/VK/tri.vert"
)
source_group("Shaders" FILES ${Shaders})

set(Shaders__Deferred
    "../../Assets/Shaders/VK/DeferredCombine.frag"
    "../../Assets/Shaders/VK/DeferredCombine.vert"
    "../../Assets/Shaders/VK/DeferredLight.frag"
    "../../Assets/Shaders/VK/DeferredLight.vert"
    "../../Assets/Shaders/VK/DeferredScene.frag"
    "../../Assets/Shaders/VK/DeferredScene.vert"
)
source_group("Shaders\\Deferred" FILES ${Shaders__Deferred})

set(Shaders__Shadow_Mapping
    "../../Assets/Shaders/VK/ShadowFill.frag"
    "../../Assets/Shaders/VK/ShadowFill.vert"
    "../../Assets/Shaders/VK/ShadowUse.frag"
    "../../Assets/Shaders/VK/ShadowUse.vert"
)
source_group("Shaders\\Shadow Mapping" FILES ${Shaders__Shadow_Mapping})

set(Source_Files
    "BasicComputeUsage.cpp"
    "BasicDescriptorRenderer.cpp"
    "BasicGeometryRenderer.cpp"
    "BasicMultiPassRenderer.cpp"
    "BasicMultiPipelineRenderer.cpp"
    "BasicPushConstantRenderer.cpp"
    "BasicSkinningExample.cpp"
    "BasicTexturingRenderer.cpp"
    "BasicUniformBufferRenderer.cpp"
    "CubeMapRenderer.cpp"
    "DeferredExample.cpp"
    "GLTFExample.cpp"
    "LightingExample.cpp"
    "MultiViewportExample.cpp"
    "PostProcessingExample.cpp"
    "Precompiled.cpp"
    "PrerecordedCmdListRenderer.cpp"
    "ShadowmappingExample.cpp"
    "TestRayTrace.cpp"
    "VulkanMain.cpp"
    "VulkanTutorialRenderer.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Shaders}
    ${Shaders__Deferred}
    ${Shaders__Shadow_Mapping}
    ${Source_Files}
)



################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Precompiled.h>"
)

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE VulkanTest)

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
        "_DEBUG;"
        "VK_PROTOTYPES"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "_CONSOLE;"
    "UNICODE;"
    "_UNICODE"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
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
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    Common
    GLTFLoader
    VulkanRendering
)

target_include_directories (${PROJECT_NAME} 
    PUBLIC ${Vulkan_INCLUDE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/Plugins/GLTFLoader
)	

foreach (file ${ALL_FILES})
    get_filename_component(file_name ${file} NAME)
	get_filename_component(file_ext ${file} EXT)
	
	set(COMPILE_SHADER
		FALSE
	)
	if(file_ext STREQUAL ".cpp" OR file_ext STREQUAL ".cpp")
		continue()
	endif()
		
	if( file_ext STREQUAL  ".vert" OR
		file_ext STREQUAL  ".frag" OR
		file_ext STREQUAL  ".comp" OR
		file_ext STREQUAL  ".geom" OR
		file_ext STREQUAL  ".tesc" OR
		file_ext STREQUAL  ".tese" OR
		file_ext STREQUAL  ".rgen" OR
		file_ext STREQUAL  ".rint" OR
		file_ext STREQUAL  ".rahit" OR
		file_ext STREQUAL  ".rchit" OR
		file_ext STREQUAL  ".rmiss" OR
		file_ext STREQUAL  ".rcall" OR
		file_ext STREQUAL  ".task" OR
		file_ext STREQUAL  ".mesh"
	)
		set(COMPILE_SHADER
			TRUE
		)
	endif()
	
	if(${COMPILE_SHADER})
		message("Adding custom command to ${file}")
		get_filename_component(file_dir ${file} ABSOLUTE)
        set(SPIRV_OUTPUT ${file_name}.spv)
		set(SPIRV_ABS_INPUT ${file_dir})
		set(SPIRV_ABS_OUTPUT ${file_dir}.spv)
		
		add_custom_command(
			OUTPUT ${SPIRV_ABS_OUTPUT}
			
			COMMENT "Compiling GLSL shader:"
			COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V  ${SPIRV_ABS_INPUT} -o ${SPIRV_ABS_OUTPUT}
            DEPENDS ${file}
			VERBATIM
		)
        list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})   
	endif()
endforeach()

add_custom_target(
    Shaders
    DEPENDS ON ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "Common;"
    "VulkanRendering;"
    "GLTFLoader"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")