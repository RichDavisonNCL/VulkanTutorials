#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in Vertex {
	vec2 texCoord;
	flat int texIndex;
} IN;

layout (location = 0) out vec4 fragColor;

layout (set = 2, binding = 0) uniform sampler2D objectTextures[];

void main() {
   fragColor 	= texture(objectTextures[IN.texIndex], IN.texCoord);
}
