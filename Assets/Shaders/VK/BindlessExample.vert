/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 position;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out Vertex {
	vec2 texCoord;
	flat int texIndex;
} OUT;

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout(set = 1, binding = 0,std140) buffer MatricesBuffer { 
    mat4 matrices[]; 
};

void main() {
   OUT.texCoord = texCoord;
   OUT.texIndex = gl_InstanceIndex;

   gl_Position 	= projMatrix * viewMatrix * matrices[gl_InstanceIndex] * vec4(position.xyz, 1);
}
