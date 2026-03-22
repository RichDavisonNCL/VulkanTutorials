/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series
Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "../NCLCoreClasses/Window.h"

#include "VulkanTutorial.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

int main(int argc, char* argv[]) {
	NCL::WindowInitialisation winInit = {
		.width			= 1120,
		.height			= 768,
		.windowTitle	= "Welcome to Vulkan!",
	};

	NCL::Vulkan::VulkanInitialisation vkInit = VulkanTutorial::DefaultInitialisation();
	std::string example = "MyFirstTriangle";

	bool lockCursor = false;
	bool runTests = false;

	for (int i = 0; i < argc; ++i) {
		std::string argument(argv[i]);
		for (char& c : argument) c = (char)tolower(c);
		if (argument == "-runtests" || argument == "-t") {
			runTests = true;
		}
		if (argument == "-example" || argument == "-e") {
			example = argv[++i];
		}
		else if (argument == "-screenwidth" || argument == "-w") {
			winInit.width = std::stoi(argv[++i]);
		}
		else if (argument == "-screenheight" || argument == "-h") {
			winInit.height = std::stoi(argv[++i]);
		}
		else if (argument == "-screenx" || argument == "-x") {
			winInit.windowPositionX = std::stoi(argv[++i]);
		}
		else if (argument == "-screeny" || argument == "-y") {
			winInit.windowPositionY = std::stoi(argv[++i]);
		}
		else if (argument == "-consolex") {
			winInit.consolePositionX = std::stoi(argv[++i]);
		}
		else if (argument == "-consoley") {
			winInit.consolePositionY = std::stoi(argv[++i]);
		}
		else if (argument == "-refreshrate") {
			winInit.refreshRate = std::stoi(argv[++i]);
		}
		else if (argument == "-fullscreen") {
			winInit.fullScreen = true;
		}
		else if (argument == "-forceintegratedgpu") {
			vkInit.idealGPU = vk::PhysicalDeviceType::eIntegratedGpu;
		}
		else if (argument == "-lockcursor") {
			lockCursor = true;
		}
	}

	vkInit.useHDRSurface = true;

	Window* w = Window::CreateGameWindow(winInit);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->LockMouseToWindow(lockCursor);

	int returnState = 0;

	if (runTests) {
		int tutorialID = 0;
		while (true) {
			auto tutorial = std::unique_ptr<VulkanTutorial>(VulkanTutorial::CreateTutorial(tutorialID, vkInit));
			if (!tutorial) {
				break;
			}
			w->SetWindowEventHandler(
				[&](WindowEvent e, uint32_t w, uint32_t h) {
					tutorial->WindowEventHandler(e, w, h);
				}
			);
			int frameID = 0;

			while (w->UpdateWindow() && frameID < 5) {
				tutorial->RunFrame(w->GetTimer().GetTimeDeltaSeconds());
				frameID++;
			}
			tutorial->Finish();
		}
	}
	else {
		auto tutorial = std::unique_ptr<VulkanTutorial>(VulkanTutorial::CreateTutorial(example, vkInit));

		w->SetWindowEventHandler(
			[&](WindowEvent e, uint32_t w, uint32_t h) {
				tutorial->WindowEventHandler(e, w, h);
			}
		);

		while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
			tutorial->RunFrame(w->GetTimer().GetTimeDeltaSeconds());
		}
		tutorial->Finish();
	}

	Window::DestroyGameWindow();

	return returnState;
}