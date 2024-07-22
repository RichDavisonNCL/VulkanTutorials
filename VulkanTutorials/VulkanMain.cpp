/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series
Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "../NCLCoreClasses/Window.h"
#include "DescriptorExample.h"
#include "TexturingExample.h"
#include "MyFirstTriangle.h"
#include "PushConstantExample.h"
#include "UniformBufferExample.h"
#include "MultiPipelineExample.h"
#include "PrerecordedCmdListExample.h"
#include "DescriptorBufferExample.h"
#include "PushDescriptorExample.h"

#include "AsyncComputeExample.h"

#include "MultiViewportExample.h"

#include "ComputeExample.h"

#include "PostProcessingExample.h"
#include "ShadowMappingExample.h"

#include "LightingExample.h"
#include "DeferredExample.h"
#include "GLTFExample.h"
#include "BindlessExample.h"

#include "SkinningExample.h"

#include "CubeMapExample.h"

#include "TessellationExample.h"
#include "GeometryShaderExample.h"
#include "OffsetUniformBufferExample.h"
#include "ComputeSkinningExample.h"

#ifdef USE_RAY_TRACING
#include "../VulkanRayTracing/TestRayTrace.h"
#include "../VulkanRayTracing/TestGLTFRayTrace.h"
#include "../VulkanRayTracing/TestRayTracedSkinning.h"
#endif

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

int main() {
	Window* w = Window::CreateGameWindow("Welcome to Vulkan!", 1120, 768);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->SetConsolePosition(2000, 200);

	/*
	API Functionality Tutorials
	*/
	//auto* tutorial = new MyFirstTriangle (*w);
	//auto* tutorial = new PushConstantExample(*w);
	//auto* tutorial = new DescriptorExample(*w);
	//auto* tutorial = new TexturingExample(*w);
	//auto* tutorial = new UniformBufferExample(*w);
	//auto* tutorial = new PrerecordedCmdListRenderer(*w);
    //auto* tutorial = new MultiPipelineExample(*w);
	
	//auto* tutorial = new MultiViewportExample(*w);
	//auto* tutorial = new BindlessExample(*w);
	//auto* tutorial = new DescriptorBufferExample(*w);
	//auto* tutorial = new PushDescriptorExample(*w);
	//auto* tutorial = new OffsetUniformBufferExample(*w);
	
	/*
	Rendering Technique Tutorials
	*/
	//auto* tutorial = new PostProcessingExample(*w);
	//auto* tutorial = new ShadowMappingExample(*w);
	//auto* tutorial = new LightingExample(*w);
	//auto* tutorial = new DeferredExample(*w);
	//auto* tutorial = new GLTFExample(*w);
	//auto* tutorial = new SkinningExample(*w);
	//auto* tutorial = new CubeMapExample(*w);

	//auto* tutorial = new TessellationExample(*w);
	//auto* tutorial = new GeometryShaderExample(*w);
	//auto* tutorial = new ComputeExample(*w);
	//auto* tutorial = new AsyncComputeExample(*w);
	//auto* tutorial = new ComputeSkinningExample(*w);

#ifdef USE_RAY_TRACING
	/*
	Ray Tracing Tutorials
	*/
	//auto* tutorial = new TestRayTrace(*w);
	auto* tutorial = new TestGLTFRayTrace(*w);
	//auto* tutorial = new TestRayTracedSkinning(*w);
#endif

	w->LockMouseToWindow(true);
	w->ShowOSPointer(false);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		tutorial->RunFrame(w->GetTimer().GetTimeDeltaSeconds());
	}

	delete tutorial;

	Window::DestroyGameWindow();
}