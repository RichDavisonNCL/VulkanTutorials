#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding  = 0, set = 2) uniform  sampler2D sceneTex;  //Diff descriptor
layout (binding  = 0, set = 3) uniform  sampler2D shadowTex; //Diff descriptor

layout (location = 0) in vec2 outTex;
layout (location = 1) in vec4 cameraProjPos;

layout (location = 0) out vec4 fragColor;

void main() {
    vec3 shadowNDC      = cameraProjPos.xyz / cameraProjPos.w;
    vec2 sampleCoord    = vec2(shadowNDC.x, 1.0f - shadowNDC.y);
    float shadowSample 	= (texture(shadowTex, sampleCoord).r);

    float offset        = -0.001f;
    fragColor 	        = vec4(1.0f,1.0f, 1.0f, 1.0f);

    if(shadowNDC.x > 0.0f && shadowNDC.x < 1.0f && 
       shadowNDC.y > 0.0f && shadowNDC.y < 1.0f && 
       abs(shadowNDC.z) > 0.0f) {
        if(shadowSample < shadowNDC.z + offset) {
            fragColor = vec4(1,0,0,1);
        }
    }
    else {
        fragColor.xyz = vec3(1);
    }
}