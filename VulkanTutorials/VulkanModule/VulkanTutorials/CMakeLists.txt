set(PROJECT_NAME VulkanTutorials)

################################################################################
# Source groups
################################################################################

file(GLOB_RECURSE Header_Files CONFIGURE_DEPENDS *.h)
source_group("Header Files" FILES ${Header_Files})

file(GLOB_RECURSE Header_Files CONFIGURE_DEPENDS *.cpp)
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
    ${Header_Files}
    ${Source_Files}
	${ALL_SHADERS}
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
		COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V  ${SPIRV_ABS_INPUT} -o ${SPIRV_ABS_OUTPUT}
		DEPENDS ${file}
		VERBATIM
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})   
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