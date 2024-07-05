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
			.width = 1120,
			.height = 768,
			.windowTitle = "Welcome to Vulkan!",
	};

	NCL::Vulkan::VulkanInitialisation vkInit = VulkanTutorial::DefaultInitialisation();

	std::string example = "MyFirstTriangle";

	for (int i = 0; i < argc; ++i) {
		std::string argument(argv[i]);
		if (argument == "-Example") {
			example = argv[++i];
		}
		else if (argument == "-ScreenWidth") {
			winInit.width = std::stoi(argv[++i]);
		}
		else if (argument == "-ScreenHeight") {
			winInit.height = std::stoi(argv[++i]);
		}
		else if (argument == "-ScreenX") {
			winInit.windowPositionX = std::stoi(argv[++i]);
		}
		else if (argument == "-ScreenY") {
			winInit.windowPositionY = std::stoi(argv[++i]);
		}
		else if (argument == "-ConsoleX") {
			winInit.consolePositionX = std::stoi(argv[++i]);
		}
		else if (argument == "-ConsoleY") {
			winInit.consolePositionY = std::stoi(argv[++i]);
		}
		else if (argument == "-RefreshRate") {
			winInit.refreshRate = std::stoi(argv[++i]);
		}
		else if (argument == "-FullScreen") {
			winInit.fullScreen = true;
		}
		else if (argument == "-ForceIntegratedGPU") {
			vkInit.idealGPU = vk::PhysicalDeviceType::eIntegratedGpu;
		}
	}

	Window* w = Window::CreateGameWindow(winInit);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->LockMouseToWindow(true);
	w->ShowOSPointer(false);

	VulkanTutorial* tutorial = VulkanTutorial::CreateTutorial(example, vkInit);

	w->SetWindowEventHandler(
		[&](WindowEvent e, uint32_t w, uint32_t h) {
			tutorial->WindowEventHandler(e, w, h);
		}
	);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		tutorial->RunFrame(w->GetTimer().GetTimeDeltaSeconds());
	}

	delete tutorial;

	Window::DestroyGameWindow();
}