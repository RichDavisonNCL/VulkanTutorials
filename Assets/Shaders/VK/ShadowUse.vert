/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec2 outTex;

layout (location = 1) out vec4 cameraProjPos;

layout (binding  = 0, set = 0) uniform  CameraMatrices 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout (binding  = 0, set = 1) uniform  ShadowMatrix 
{
	mat4 shadowMatrix;
};

layout(push_constant) uniform PushConstantVert{
	mat4 modelMatrix;
};

const mat4 biasMattrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
   outTex 	= texCoord;

   vec4 worldPos = modelMatrix * vec4(pos.xyz, 1);
   
   cameraProjPos    = biasMattrix * shadowMatrix * worldPos;
   gl_Position 	    = projMatrix  * viewMatrix   * worldPos;
}
