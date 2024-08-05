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
#extension GL_EXT_nonuniform_qualifier : enable

#include "RayStructs.glslh"
#include "SceneNode.glslh"

layout(location = 0) rayPayloadInEXT BasicPayload payload;
//layout(binding  = 0, set = 4) uniform  sampler2D tex1; //Default Sampler descriptor

layout(binding = 0, set = 4) buffer VertexPositionBuffer 
{
    vec3 positions[];
} vertexPositionBuffer;

layout(binding = 1, set = 4) buffer IndexBuffer 
{
    int indices[];
} indicesBuffer;

layout(binding = 2, set = 4) buffer VertexTexCordBuffer 
{
    vec2 textureCoords[];
} vertexTexCoords;

layout(binding = 3, set = 4) buffer VertexNormalBuffer 
{
    vec3 normals[];
} vertexNormals;

layout(binding  = 4, set = 4) uniform  texture2D textureMap[]; //Default Sampler descriptor
layout(binding = 5, set = 4) buffer MatlayerBuffer 
{
    GLTFMaterialLayer matLayerList[];
} matlayerBuffer;

layout(binding = 6, set = 4) buffer PrimInfoBuffer 
{
    GLTFPrimInfo primeInfoList[];
} primeInfoBuffer;

layout(binding  = 0, set = 5) uniform sampler mySampler; //Default Sampler descriptor

hitAttributeEXT vec2 hitBarycentrics;
void main() 
{

    // Retrieve the Primitive mesh buffer information
    // gl_InstanceCustomIndexEXT we are setting it as mesh ID check sample
    GLTFPrimInfo pinfo = primeInfoBuffer.primeInfoList[gl_InstanceCustomIndexEXT + gl_GeometryIndexEXT];

    // Getting the 'first index' for this mesh (offset of the mesh + offset of the triangle)
    uint indexOffset  = pinfo.indexOffset + (3 * gl_PrimitiveID);
    uint vertexOffset = pinfo.vertexOffset; 
    uint matIndex     = max(0, pinfo.materialLayerID);  // material of primitive mesh
    GLTFMaterialLayer material = matlayerBuffer.matLayerList[matIndex];
  
    ivec3 index = ivec3(
        indicesBuffer.indices[0 + indexOffset],
        indicesBuffer.indices[1 + indexOffset],
        indicesBuffer.indices[2 + indexOffset]);
    index += ivec3(vertexOffset);

    const vec3 barycentrics = vec3(
        1 - hitBarycentrics.x - hitBarycentrics.y,
        hitBarycentrics.x,
        hitBarycentrics.y);// (w,u,v)

    const vec2 uv0 = vertexTexCoords.textureCoords[index.x];
    const vec2 uv1 = vertexTexCoords.textureCoords[index.y];
    const vec2 uv2 = vertexTexCoords.textureCoords[index.z];

    // Vertex of the triangle
    const vec3 pos0 = vertexPositionBuffer.positions[index.x];
    const vec3 pos1 = vertexPositionBuffer.positions[index.y];
    const vec3 pos2 = vertexPositionBuffer.positions[index.z];

    // Normal
    const vec3 nrm0 = vertexNormals.normals[index.x];
    const vec3 nrm1 = vertexNormals.normals[index.y];
    const vec3 nrm2 = vertexNormals.normals[index.z];

    //https://computergraphics.stackexchange.com/questions/7738/how-to-assign-calculate-triangle-texture-coordinates
    const vec2 texCoord = (uv0 * barycentrics.x) +
                          (uv1 * barycentrics.y) +
                          (uv2 * barycentrics.z);


    const vec3 position = pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
    const vec3 world_position = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

    const vec3 normal = normalize(nrm0 * barycentrics.x + nrm1 * barycentrics.y + nrm2 * barycentrics.z);
    const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));
    const vec3 geom_normal  = normalize(cross(pos1 - pos0, pos2 - pos0));

    vec3 textureColor = texture(sampler2D(textureMap[material.albedoId], mySampler), texCoord).xyz;
    vec3 normalTexture = texture(sampler2D(textureMap[material.bumpId], mySampler), texCoord).xyz;
    payload.hitValue = textureColor;
}