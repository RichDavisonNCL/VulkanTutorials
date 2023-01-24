#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 colour;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out Vertex {
	vec2 texCoord;
} OUT;

void main() {
   OUT.texCoord = texCoord;

   gl_Position 	= position;
}
