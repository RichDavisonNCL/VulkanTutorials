/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#version 460
#extension GL_GOOGLE_include_directive		: enable
#extension GL_ARB_separate_shader_objects	: enable
#extension GL_ARB_shading_language_420pack	: enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_tracing_position_fetch : require

#include "RayStructs.glslh"

layout(location = 0) rayPayloadInEXT BasicPayload payload;
layout(binding  = 0, set = 4) uniform  sampler2D tex1; //Default Sampler descriptor

layout(binding = 1, set = 5) buffer VertexPositionBuffer 
{
    vec3 positions[];
} vertexPositionBuffer;

layout(binding = 1, set = 6) buffer VertexTexCordBuffer 
{
    vec2 textureCoords[];
} vertexTexCoords;

layout(binding = 1, set = 7) buffer IndexBuffer 
{
    int indices[];
} indicesBuffer;

hitAttributeEXT vec2 hitBarycentrics;
void main() 
{
	uint index0 = indicesBuffer.indices[gl_PrimitiveID * 3 + 0];
    uint index1 = indicesBuffer.indices[gl_PrimitiveID * 3 + 1];
    uint index2 = indicesBuffer.indices[gl_PrimitiveID * 3 + 2];

	// Fetch the texture coordinates of the hit triangle's vertices
    vec2 texCoord0 = vertexTexCoords.textureCoords[index0];
    vec2 texCoord1 = vertexTexCoords.textureCoords[index1];
    vec2 texCoord2 = vertexTexCoords.textureCoords[index2];

	// Calculate the third barycentric coordinate
    float w = 1.0 - hitBarycentrics.x - hitBarycentrics.y;

    // Interpolate the texture coordinates using barycentric coordinates
    vec2 interpolatedTexCoord = hitBarycentrics.x * texCoord0 +
                                hitBarycentrics.y * texCoord1 +
                                w * texCoord2;

    // Sample the texture
    vec3 textureColor = texture(tex1, interpolatedTexCoord).xyz;

    // Set the ray payload to the sampled texture color
    payload.hitValue = textureColor;
}