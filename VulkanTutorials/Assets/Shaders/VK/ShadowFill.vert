#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 pos;

layout(push_constant) uniform PushConstantVert{
	mat4 modelMatrix;
};

layout (binding  = 0) uniform  ShadowInfo 
{
	mat4 shadowMatrix;
};

void main() {
   gl_Position 	= shadowMatrix * modelMatrix * vec4(pos.xyz, 1);
}
