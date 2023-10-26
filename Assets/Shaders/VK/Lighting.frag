/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec3 inWorldPos;

layout (set = 1, binding  = 0) uniform Light 
{
	vec3	lightPosition;
	float	lightRadius;
	vec4	lightColour;
};

layout (set = 2, binding  = 0) uniform  sampler2D diffuseTex;
layout (set = 2, binding  = 1) uniform  sampler2D bumpTex;

layout (set = 3, binding  = 0) uniform CameraPos 
{
	vec3	cameraPosition;
};

layout (location = 0) out vec4 fragColor;

void main() {
	vec4 diffuse	= texture(diffuseTex, inTexCoord);
	vec4 bump		= texture(bumpTex, inTexCoord);

	vec3 normal		= normalize(inNormal);
	vec3 tangent	= normalize(inTangent);
	vec3 binormal	= cross(normal, tangent);

	mat3 tbn = mat3(tangent, binormal, normal);

	vec3 worldBump = tbn * normalize(bump.xyz * 2 - 1); 

	vec3 incident	= normalize(lightPosition - inWorldPos);
	vec3 viewDir	= normalize(cameraPosition - inWorldPos);
	vec3 halfAngle	= normalize(incident + viewDir);

	float fragDistance	= length(lightPosition - inWorldPos);
	float attenuation	= 1.0f / (fragDistance * fragDistance);

	float ambient	= 0.025f;
	float lambert	= clamp(dot(worldBump, incident), ambient, 1);
	float intensity = 1000.0f;

    fragColor = diffuse * lightColour * lambert * attenuation * intensity;

	float specAmount = dot(worldBump, halfAngle);
	float specPower  = clamp(pow(specAmount, 40.0f), 0.0f, 1.0f);

	fragColor.xyz += lightColour.xyz * specPower * attenuation * intensity; ; //specular!
}