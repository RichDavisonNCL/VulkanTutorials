#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
//layout (location = 1) in vec4 inColour;
layout (location = 2) in vec2 inTexCoord;

layout (location = 5) in vec4	inJointWeight;
layout (location = 6) in ivec4	inJointIndex;

layout (location = 0) out vec2 texcoord;

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout (set = 2, binding  = 0) buffer jointData 
{
	mat4 allJoints[];
};

void main() {
   texcoord 	= inTexCoord;

   mat4 skinMat = (allJoints[inJointIndex[0]] * inJointWeight[0]) +
				  (allJoints[inJointIndex[1]] * inJointWeight[1]) +
				  (allJoints[inJointIndex[2]] * inJointWeight[2]) +
				  (allJoints[inJointIndex[3]] * inJointWeight[3]);
				  
   gl_Position = projMatrix * viewMatrix * skinMat * vec4(inPos.xyz,1);
}
