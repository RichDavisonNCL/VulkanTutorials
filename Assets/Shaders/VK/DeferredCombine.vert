/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec2 texcoord;

void main() {
   texcoord 	= inTexCoord;
   texcoord.y = 1.0f - inTexCoord.y;
   gl_Position 	= vec4(inPosition,1);
}
