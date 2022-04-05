set(PROJECT_NAME Common)

################################################################################
# Source groups
################################################################################
set(Asset_Handling
    "Assets.cpp"
    "Assets.h"
    "SimpleFont.cpp"
    "SimpleFont.h"
    "TextureLoader.cpp"
    "TextureLoader.h"
    "TextureWriter.cpp"
    "TextureWriter.h"
)
source_group("Asset Handling" FILES ${Asset_Handling})

set(Header_Files
    "Camera.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Maths
    "Maths.cpp"
    "Maths.h"
    "Matrix2.cpp"
    "Matrix2.h"
    "Matrix3.cpp"
    "Matrix3.h"
    "Matrix4.cpp"
    "Matrix4.h"
    "Plane.cpp"
    "Plane.h"
    "Quaternion.cpp"
    "Quaternion.h"
    "Vector2.cpp"
    "Vector2.h"
    "Vector3.cpp"
    "Vector3.h"
    "Vector4.cpp"
    "Vector4.h"
)
source_group("Maths" FILES ${Maths})

set(Rendering
    "MeshAnimation.cpp"
    "MeshAnimation.h"
    "MeshGeometry.cpp"
    "MeshGeometry.h"
    "MeshMaterial.cpp"
    "MeshMaterial.h"
    "RendererBase.cpp"
    "RendererBase.h"
    "ShaderBase.cpp"
    "ShaderBase.h"
    "TextureBase.cpp"
    "TextureBase.h"
)
source_group("Rendering" FILES ${Rendering})

set(Source_Files
    "Camera.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(Windowing_and_Input
    "GameTimer.cpp"
    "GameTimer.h"
    "Keyboard.cpp"
    "Keyboard.h"
    "Mouse.cpp"
    "Mouse.h"
    "Window.cpp"
    "Window.h"
)
source_group("Windowing and Input" FILES ${Windowing_and_Input})

set(Windowing_and_Input__Win32
    "Win32Keyboard.cpp"
    "Win32Keyboard.h"
    "Win32Mouse.cpp"
    "Win32Mouse.h"
    "Win32Window.cpp"
    "Win32Window.h"
)
source_group("Windowing and Input\\Win32" FILES ${Windowing_and_Input__Win32})

set(ALL_FILES
    ${Asset_Handling}
    ${Header_Files}
    ${Maths}
    ${Rendering}
    ${Source_Files}
    ${Windowing_and_Input}
    ${Windowing_and_Input__Win32}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE Common)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "_CRT_SECURE_NO_WARNINGS;"
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

