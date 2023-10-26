/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive		: enable

#include "Lighting.glslh"

layout (location = 0) in vec3 inPosition;
//No Colour
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out int lightIndex;

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout (set = 1, binding  = 0) uniform Lights 
{
	Light allLights[64];
};

void main() {
	lightIndex			= gl_InstanceIndex;
   vec3 lightPos		= allLights[gl_InstanceIndex].position;
   float lightRadius	= allLights[gl_InstanceIndex].radius;
   gl_Position 			= projMatrix * viewMatrix * vec4((inPosition * lightRadius) + lightPos,1);
}