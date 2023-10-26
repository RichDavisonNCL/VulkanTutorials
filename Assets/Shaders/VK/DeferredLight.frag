/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects	: enable
#extension GL_ARB_shading_language_420pack	: enable
#extension GL_GOOGLE_include_directive		: enable

#include "Lighting.glslh"

layout (set = 1, binding  = 0) uniform Lights 
{
	Light allLights[64];
};

layout (set = 2, binding  = 0) uniform LightStageUBO 
{	
	mat4	inverseProjView;	
	vec3	cameraPosition;
	float	scrap; //Maintains Alignment
	vec2	resolution;
};

layout (location = 0) in flat int lightIndex;

layout (set = 3, binding  = 0) uniform  sampler2D bumpTex;	//Comes from G-Buffer
layout (set = 3, binding  = 1) uniform  sampler2D depthTex;	//Comes from G-Buffer

layout (location = 0) out vec4 diffuseColour;
layout (location = 1) out vec4 specularColour;

void main() {
	vec2 texCoord	= gl_FragCoord.xy * resolution.xy;

	vec3 worldBump		= normalize(texture(bumpTex , texCoord).xyz * 2 - 1); 
	float depthSample	= texture(depthTex, texCoord).r;

	vec3 ndcSpace = gl_FragCoord.xyz;
	ndcSpace.z = depthSample;	//Replace depth with the fragment's original depth from GBuffer
	ndcSpace.xy *= resolution;	//brings it to 0 to 1 range
	ndcSpace.xy *= 2.0f;		//now its 0 to 2!
	ndcSpace.xy -= 1.0f;		// and now its -1 to 1! //TODO - z????
	ndcSpace.y *= -1;
	vec4 clipSpace  = inverseProjView * vec4(ndcSpace, 1);
	vec3 worldSpace = clipSpace.xyz /= clipSpace.w;

	float distance = length(allLights[lightIndex].position - worldSpace);

	if(distance > allLights[lightIndex].radius) {
		discard;
	}
	LightCalculation(worldSpace, worldBump, cameraPosition, vec3(1,1,1), allLights[lightIndex], diffuseColour.xyz, specularColour.xyz, 0.0f);

	diffuseColour.a		= 1.0f;
	specularColour.a	= 1.0f;
}