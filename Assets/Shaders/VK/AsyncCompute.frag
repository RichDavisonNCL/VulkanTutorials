/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////

#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) out vec4 fragColor;

layout (set = 0, binding  = 1) buffer AsyncFrameData 
{
	uint computeFrameID;
};

layout(push_constant) uniform PushFrame {
	uint pushFrameID;
};

void main() {
	if(computeFrameID == pushFrameID) {
		fragColor 	= vec4(1, 1, 1, 1);
	}
	else {
		fragColor 	= vec4(0, 0, 0, 1);
	}
}
