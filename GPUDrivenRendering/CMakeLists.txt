set(TARGET_NAME GPUDrivenRendering)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
# Exclude test files from main target
list(FILTER SOURCES EXCLUDE REGEX ".*/Tests/.*")

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/NCLCoreClasses
    ${CMAKE_SOURCE_DIR}/VulkanRendering
    ${CMAKE_SOURCE_DIR}/GLTFLoader
    ${CMAKE_SOURCE_DIR}/VulkanTutorials
)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${TARGET_NAME} PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VK_NO_PROTOTYPES
    ASSETROOTLOCATION="${CMAKE_SOURCE_DIR}/Assets/"
)

# ImGui support (optional, for interactive debug mode)
if(COMPILE_WITH_IMGUI)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_IMGUI=1)
    target_include_directories(${TARGET_NAME} PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/imgui*.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp)
    target_sources(${TARGET_NAME} PRIVATE ${IMGUI_SOURCES})
    target_sources(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/imgui/implot.cpp
        ${CMAKE_SOURCE_DIR}/imgui/implot_items.cpp
    )
endif()

# Shader compilation
file(GLOB SHADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp)

set(SHADER_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_OUTPUT ${CMAKE_SOURCE_DIR}/Assets/Shaders/VK/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_OUTPUT}
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --target-env vulkan1.3 ${SHADER} -o ${SPV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} -> ${SHADER_NAME}.spv"
        VERBATIM
    )
    list(APPEND SHADER_OUTPUTS ${SPV_OUTPUT})
endforeach()

add_custom_target(GPUDrivenRendering_Shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${TARGET_NAME} GPUDrivenRendering_Shaders)

target_link_libraries(${TARGET_NAME} PRIVATE
    NCLCoreClasses
    VulkanRendering
    GLTFLoader
)

set_target_properties(${TARGET_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ── Unit test target (no Vulkan dependency, WFC + data layout only) ──
set(TEST_TARGET GPUDrivenRendering_Tests)
add_executable(${TEST_TARGET}
    Tests/TestMain.cpp
    WFCGenerator.cpp
)
target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/NCLCoreClasses
)
target_compile_features(${TEST_TARGET} PRIVATE cxx_std_20)
target_link_libraries(${TEST_TARGET} PRIVATE NCLCoreClasses)
set_target_properties(${TEST_TARGET} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
