#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 attr;

layout(push_constant) uniform PushConstantVert{
	vec3 positionOffset;
};

layout (location = 0) out vec2 texcoord;

void main() {
   texcoord 	= attr;
   gl_Position 	= pos + vec4(positionOffset.xyz,0);
}
