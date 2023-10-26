/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 attr;

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout (set = 1, binding  = 0) uniform  ObjectInfo 
{
	mat4 modelMatrix;
};


layout (location = 0) out vec2 texcoord;

void main() {
   texcoord 	= attr;
   gl_Position 	= projMatrix * viewMatrix * modelMatrix * vec4(pos.xyz, 1);
}
