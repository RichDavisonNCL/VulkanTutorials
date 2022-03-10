#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;

layout (set = 0, binding  = 0) buffer testData 
{
	vec4 allData[];
};

void main() {

   gl_Position 	= vec4(allData[gl_VertexIndex].xyz, 1.0f);
   gl_PointSize = 0.05f;
}
