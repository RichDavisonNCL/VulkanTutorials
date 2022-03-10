#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec2 outTex;

void main() {
   outTex 	= texCoord;
   outTex.y = 1.0f - outTex.y;
   gl_Position 	= pos;
}
