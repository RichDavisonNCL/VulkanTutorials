/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series
Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "../NCLCoreClasses/Window.h"
#include "BasicDescriptorRenderer.h"
#include "BasicTexturingRenderer.h"
#include "BasicGeometryRenderer.h"
#include "BasicPushConstantRenderer.h"
#include "BasicUniformBufferRenderer.h"
#include "BasicMultiPipelineRenderer.h"
#include "PrerecordedCmdListRenderer.h"
#include "DescriptorBufferExample.h"

#include "AsyncComputeExample.h"

#include "MultiViewportExample.h"

#include "BasicComputeUsage.h"

#include "PostProcessingExample.h"
#include "ShadowMappingExample.h"

#include "LightingExample.h"
#include "DeferredExample.h"
#include "GLTFExample.h"
#include "BindlessExample.h"

#include "BasicSkinningExample.h"

#include "CubeMapRenderer.h"

#include "TessellationExample.h"
#include "GeometryShaderExample.h"
#include "OffsetUniformBufferRenderer.h"
#include "ComputeSkinningExample.h"

#ifdef USE_RAY_TRACING
#include "../VulkanRayTracing/TestRayTrace.h"
#endif

using namespace NCL;

int main() {
	Window* w = Window::CreateGameWindow("Welcome to Vulkan!", 1120, 768);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->SetConsolePosition(2000, 200);

	/*
	API Functionality Tutorials
	*/
	//auto* renderer = new BasicGeometryRenderer(*w);	
	//auto* renderer = new BasicPushConstantRenderer(*w);
	//auto* renderer = new BasicDescriptorRenderer(*w);
	//auto* renderer = new BasicTexturingRenderer(*w);
	//auto* renderer = new BasicUniformBufferRenderer(*w);
	//auto* renderer = new PrerecordedCmdListRenderer(*w);
	//auto* renderer = new BasicMultiPipelineRenderer(*w);
	//auto* renderer = new BasicComputeUsage(*w);
	//auto* renderer = new MultiViewportExample(*w);
	//auto* renderer = new BindlessExample(*w);
	//auto* renderer = new DescriptorBufferExample(*w);
	//auto* renderer = new AsyncComputeUsage(*w);
	//auto* renderer = new OffsetUniformBufferRenderer(*w);
	
	/*
	Rendering Technique Tutorials
	*/
	//auto* renderer = new PostProcessingExample(*w);
	//auto* renderer = new ShadowMappingExample(*w);
	//auto* renderer = new LightingExample(*w);
	//auto* renderer = new DeferredExample(*w);
	//auto* renderer = new GLTFExample(*w);
	//auto* renderer = new TestRayTrace(*w);
	//auto* renderer = new BasicSkinningExample(*w);
	//auto* renderer = new CubeMapRenderer(*w);

	//auto* renderer = new TessellationExample(*w);
	//auto* renderer = new GeometryShaderExample(*w);

	//auto* renderer = new AsyncComputeExample(*w);
	auto* renderer = new ComputeSkinningExample(*w);

#ifdef USE_RAY_TRACING
	/*
	Ray Tracing Tutorials
	*/
	//auto* renderer = new TestRayTrace(*w);
#endif

	if (!renderer->Init()) {
		return -1;
	}
	renderer->SetupTutorial();

	w->LockMouseToWindow(true);
	w->ShowOSPointer(false);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		renderer->Update(w->GetTimer()->GetTimeDeltaSeconds());
		renderer->Render();
	}

	delete renderer;

	Window::DestroyGameWindow();
}