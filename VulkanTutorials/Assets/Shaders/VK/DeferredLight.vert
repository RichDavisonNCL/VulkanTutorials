#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;
//No Colour
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout(push_constant) uniform LightIndex    {
    int lightIndex;
};

struct Light	{
	vec3	position;
	float	radius;
	vec4	colour;
};

layout (set = 0, binding  = 0) uniform  CameraInfo 
{
	mat4 viewMatrix;
	mat4 projMatrix;
};

layout (set = 1, binding  = 0) uniform Lights 
{
	Light allLights[64];
};

void main() {
   vec3 lightPos		= allLights[lightIndex].position;
   float lightRadius	= allLights[lightIndex].radius;
   gl_Position 			= projMatrix * viewMatrix * vec4((inPosition * lightRadius) + lightPos,1);
}
