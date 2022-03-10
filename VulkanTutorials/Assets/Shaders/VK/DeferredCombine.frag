#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in  vec2 inTexCoord;

layout (location = 0) out vec4 fragColor;

layout (set = 0, binding  = 0) uniform  sampler2D sceneDiffuse;
layout (set = 0, binding  = 1) uniform  sampler2D lightDiffuse;
layout (set = 0, binding  = 2) uniform  sampler2D lightSpecular;

void main() {
    vec4  sceneSample	    = texture(sceneDiffuse , inTexCoord);
    vec4  diffuseSample	    = texture(lightDiffuse , inTexCoord);
    vec4  specularSample	= texture(lightSpecular, inTexCoord);

   fragColor = (sceneSample * diffuseSample) + specularSample;

   //fragColor = sceneSample + diffuseSample + specularSample;
   //no specular yet
   //fragColor = sceneSample + specularSample;

  // fragColor = diffuseSample;

  //fragColor = sceneSample;
   fragColor.a = 1.0f;
}