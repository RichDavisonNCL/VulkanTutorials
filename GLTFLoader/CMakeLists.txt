set(PROJECT_NAME GLTFLoader)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "GLTFLoader.h"
    "json.hpp"
    "tiny_gltf.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "GLTFLoader.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

set(GLTF_LOADER_DIR
	${CMAKE_CURRENT_SOURCE_DIR}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

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

target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")