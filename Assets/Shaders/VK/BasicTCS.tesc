/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (vertices = 3) out;

layout (location = 0) in InputVertex {
	vec2 texCoord; //The vertex shader is emitting an attribute
} IN[];

layout (location = 0) out OutputVertex {
	vec2 texCoord;	//We'll change it to something different!
} OUT[];

layout (push_constant) uniform pushBlock {
	vec4 tessValues;
};

layout (location = 1) patch out PatchData {
	vec4 subColour;
} patchData;

void main() {

	if (gl_InvocationID == 0) {
		gl_TessLevelInner[0] = tessValues.x;
		gl_TessLevelOuter[0] = tessValues.y;
		gl_TessLevelOuter[1] = tessValues.z;
		gl_TessLevelOuter[2] = tessValues.w;
	}

	gl_out[gl_InvocationID ]. gl_Position = gl_in[gl_InvocationID ]. gl_Position;
	OUT[gl_InvocationID].texCoord = IN[gl_InvocationID].texCoord;

	patchData.subColour = vec4(1,0,1,1);
}
