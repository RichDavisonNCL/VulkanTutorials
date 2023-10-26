/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding  = 0, set = 0) uniform  sampler2D tex;

layout (location = 0) in  vec2 texcoord;
layout (location = 0) out vec4 fragColor;

void main() {
   fragColor 	= texture(tex, texcoord);
}