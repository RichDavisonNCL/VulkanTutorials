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

//#include "AsyncComputeUsage.h"

#include "MultiViewportExample.h"

#include "BasicComputeUsage.h"

#include "PostProcessingExample.h"
#include "ShadowMappingExample.h"

#include "LightingExample.h"
#include "DeferredExample.h"
#include "TestRayTrace.h"
#include "GLTFExample.h"
#include "BindlessExample.h"

#include "BasicSkinningExample.h"

#include "CubeMapRenderer.h"

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
	//BasicGeometryRenderer* renderer = new BasicGeometryRenderer(*w);	
	//BasicPushConstantRenderer* renderer = new BasicPushConstantRenderer(*w);
	//BasicDescriptorRenderer* renderer = new BasicDescriptorRenderer(*w);
	//BasicTexturingRenderer* renderer = new BasicTexturingRenderer(*w);
	//BasicUniformBufferRenderer* renderer = new BasicUniformBufferRenderer(*w);
	//PrerecordedCmdListRenderer* renderer = new PrerecordedCmdListRenderer(*w);
	//BasicMultiPipelineRenderer* renderer = new BasicMultiPipelineRenderer(*w);
	//BasicComputeUsage * renderer = new BasicComputeUsage(*w);
	//MultiViewportExample* renderer = new MultiViewportExample(*w);
	//BindlessExample* renderer = new BindlessExample(*w);

	//AsyncComputeUsage* renderer = new AsyncComputeUsage(*w);

	/*
	Rendering Technique Tutorials
	*/
	//PostProcessingExample* renderer = new PostProcessingExample(*w);
	//ShadowMappingExample* renderer = new ShadowMappingExample(*w);
	//LightingExample* renderer = new LightingExample(*w);
	DeferredExample* renderer = new DeferredExample(*w);
	//GLTFExample* renderer = new GLTFExample(*w);
	//TestRayTrace* renderer = new TestRayTrace(*w);
	//BasicSkinningExample* renderer = new BasicSkinningExample(*w);
	//CubeMapRenderer* renderer = new CubeMapRenderer(*w);
	//DescriptorBufferExample* renderer = new DescriptorBufferExample(*w);

	if (!renderer->Init()) {
		return -1;
	}
	renderer->SetupTutorial();

	w->LockMouseToWindow(true);
	w->ShowOSPointer(false);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float time = w->GetTimer()->GetTimeDeltaSeconds();
		renderer->Update(time);
		renderer->Render();
	}

	delete renderer;

	Window::DestroyGameWindow();
}