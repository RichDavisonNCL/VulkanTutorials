#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
//layout (location = 1) in vec4 inColour;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec2 texcoord;

void main() {
   texcoord 		= inTexCoord;
   gl_Position 		= inPos;
}
