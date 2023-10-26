/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;
//No Colour
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;


layout (location = 2) out vec2 outTexCoord;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out vec3 outTangent;
layout (location = 5) out vec3 outWorldPos;

layout(push_constant) uniform PushConstantVert{
	mat4 modelMatrix;
};

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

void main() {
	vec4 worldPos	= modelMatrix * vec4(inPosition.xyz, 1);
	gl_Position 	= projMatrix * viewMatrix * worldPos;

	mat3 normalMatrix = inverse(transpose(mat3(modelMatrix)));

	outWorldPos = worldPos.xyz;
	outTexCoord = inTexCoord;
	outNormal	= normalMatrix * inNormal;
	outTangent	= normalMatrix * inTangent;
}
