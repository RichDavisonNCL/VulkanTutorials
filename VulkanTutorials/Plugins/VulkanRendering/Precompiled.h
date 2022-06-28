#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VULKAN_HPP_ASSERT
#include "vulkan/vulkan.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <iosfwd>
#include <set>


#include "../../Common/Matrix2.h"
#include "../../Common/Matrix3.h"
#include "../../Common/Matrix4.h"

#include "../../Common/Vector2.h"
#include "../../Common/Vector3.h"
#include "../../Common/Vector4.h"

#include "../../Common/Quaternion.h"