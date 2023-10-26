/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding  = 0) buffer testData 
{
	vec4 allData[];
};

void main() {
   gl_Position 	= vec4(allData[gl_VertexIndex].xyz, 1.0f);
   gl_PointSize = 0.05f;
}
