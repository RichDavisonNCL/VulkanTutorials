#include "Window.h"

#ifdef _WIN32
#include "Win32Window.h"
#endif

#ifdef __ORBIS__
#include "../Plugins/PlayStation4/PS4Window.h"
#endif

#include "RendererBase.h"
#include "GameTimer.h"

using namespace NCL;
using namespace Rendering;

Window*		Window::window		= nullptr;
Keyboard*	Window::keyboard	= nullptr;
Mouse*		Window::mouse		= nullptr;
GameTimer	Window::timer;

Window::Window()	{
	window		= this;
}

Window::~Window()	{
	delete keyboard;keyboard= nullptr;
	delete mouse;	mouse	= nullptr;
	window = nullptr;
}

Window* Window::CreateGameWindow(const WindowInitialisation& init) {
	if (window) {
		return nullptr;
	}
#ifdef _WIN32
	return new Win32Code::Win32Window(init);
#endif
	return nullptr;
}

bool	Window::UpdateWindow() {
	std::this_thread::yield();
	timer.Tick();

	if (mouse) {
		mouse->UpdateFrameState(timer.GetTimeDeltaMSec());
	}
	if (keyboard) {
		keyboard->UpdateFrameState(timer.GetTimeDeltaMSec());
	}

	return InternalUpdate();
}
