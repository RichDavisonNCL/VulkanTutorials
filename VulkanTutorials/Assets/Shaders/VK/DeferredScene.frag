#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
//#extension GL_KHR_vulkan_glsl : enable

layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec3 inWorldPos;

layout (location = 0) out vec4 gBufferA;
layout (location = 1) out vec4 gBufferB;

layout (set = 1, binding  = 0) uniform  sampler2D diffuseTex;
layout (set = 1, binding  = 1) uniform  sampler2D bumpTex;

void main() {
	vec4 diffuse	= texture(diffuseTex, inTexCoord);
	vec4 bump		= texture(bumpTex, inTexCoord);

	vec3 normal		= normalize(inNormal);
	vec3 tangent	= normalize(inTangent);
	vec3 binormal	= cross(normal, tangent);

	mat3 tbn = mat3(tangent, binormal, normal);

	vec3 worldBump = tbn * normalize(bump.xyz * 2 - 1); 

	gBufferA		= diffuse;
	gBufferB.xyz	= worldBump * 0.5 + 0.5f; //converts from -1/1 to 0/1
	gBufferB.a		= 1.0f;
}