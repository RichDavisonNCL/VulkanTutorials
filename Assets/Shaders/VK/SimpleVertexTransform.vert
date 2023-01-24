#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 texCoords;

layout (binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout(push_constant) uniform PushConstantVert{
	mat4 modelMatrix;
};

layout (location = 0) out vec2 texcoord;

void main() {
   texcoord 	= texCoords;
   gl_Position 	= projMatrix * viewMatrix * modelMatrix * vec4(pos.xyz, 1);
}
