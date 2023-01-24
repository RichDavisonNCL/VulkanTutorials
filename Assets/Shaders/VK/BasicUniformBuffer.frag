#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in  vec2 texcoord;

layout (location = 0) out vec4 fragColor;

void main() {
   fragColor 	= vec4(1,1,1,1);
}