/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inWorldPos;

layout (location = 0) out vec4 fragColor;

layout (set  = 1, binding = 0) uniform  samplerCube cubeTex;

layout (set = 2, binding  = 0) uniform CameraPos 
{
	vec3	cameraPosition;
};

void main() {
	vec3 worldDir = normalize(inWorldPos - cameraPosition);
   fragColor 		= texture(cubeTex, reflect(worldDir,inNormal));
}