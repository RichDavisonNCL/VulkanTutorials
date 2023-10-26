/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles, equal_spacing, ccw) in;

layout (location = 0) in InputVertex {
	vec2 texCoord;
} IN[];

layout (location = 0) out OutputVertex {
	vec2 texCoord;
} OUT;

layout (location = 1) patch in PatchData {
	vec4 subColour;
} patchData;


void main() {
	vec3 p0 = gl_TessCoord.x * gl_in [0].gl_Position.xyz;
	vec3 p1 = gl_TessCoord.y * gl_in [1].gl_Position.xyz;
	vec3 p2 = gl_TessCoord.z * gl_in [2].gl_Position.xyz;

	vec3 combinedPos = p0 + p1 + p2;

	//OUT.texCoord = IN.texCoord;// * subColour.xy;

	OUT.texCoord = vec2(1,1);

	gl_Position = vec4(combinedPos, 1.0f);
}
