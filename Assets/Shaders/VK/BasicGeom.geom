/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (location = 0) in InputVertex {
	vec2 unused; //The vertex shader is emitting an attribute
} IN[];

layout (location = 0) out OutputVertex {
	vec2 texCoord;	//We'll change it to something different!
} OUT;

void main() {
	float newSize = 0.25f;
	vec4 centre = vec4(gl_in[0].gl_Position.xyz * newSize, gl_in[0].gl_Position.w);

	//Now we'll emit a tri per point
	gl_Position = centre + vec4( newSize, -newSize, 0.0, 0.0);
	OUT.texCoord = vec2(1.0f, 0.0f);
	EmitVertex();

	gl_Position = centre + vec4( -newSize, -newSize, 0.0, 0.0);
	OUT.texCoord = vec2(1.0f, 1.0f);
	EmitVertex();

	gl_Position = centre + vec4( 0.0, newSize, 0.0, 0.0);
	OUT.texCoord = vec2(0.0f, 1.0f);
	EmitVertex();

	EndPrimitive();
}
