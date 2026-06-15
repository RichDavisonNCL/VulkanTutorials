set(PROJECT_NAME NCLCoreClasses)

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

    "Plane.cpp"
    "Plane.h"
	"Frustum.cpp"
    "Frustum.h"
    "Quaternion.cpp"
    "Quaternion.h"

	"Vector.h"
    "Matrix.h"
)
source_group("Maths" FILES ${Maths})

set(Rendering
    "MeshAnimation.cpp"
    "MeshAnimation.h"
    "Mesh.cpp"
    "Mesh.h"

    "Buffer.h"
	
	"MshLoader.cpp"
    "MshLoader.h"
	
    "MeshMaterial.cpp"
    "MeshMaterial.h"
    "RendererBase.cpp"
    "RendererBase.h"
    "Shader.cpp"
    "Shader.h"
    "Texture.cpp"
    "Texture.h"
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
	"Controller.cpp"
    "Controller.h"
	"KeyboardMouseController.cpp"
    "KeyboardMouseController.h"
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

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <memory>
    <unordered_set>
    <vector>
    <string>
    <fstream>
    <sstream>
    <iostream>
    <string>
    <iosfwd>
    <set>
    <map> 
    <chrono>
    <thread>
    <filesystem>
    <functional>
	<algorithm>
	<assert.h>  
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