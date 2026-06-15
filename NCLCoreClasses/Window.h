/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Keyboard.h"
#include "Mouse.h"

#include "Vector.h"

namespace NCL {
	class GameTimer;
	namespace Rendering {
		class RendererBase;
	};
	using namespace Rendering;

	enum class WindowEvent {
		Minimize,
		Maximize,
		Resize,
		Fullscreen,
		Windowed
	};

	struct WindowInitialisation {
		uint32_t width;
		uint32_t height;
		bool	 fullScreen			= false;
		uint32_t refreshRate		= 60;

		std::string windowTitle		= "NCLGL!";

		uint32_t windowPositionX	= 0;
		uint32_t windowPositionY	= 0;
		uint32_t consolePositionX	= 0;
		uint32_t consolePositionY	= 0;
	};

	using WindowEventHandler = std::function<void(WindowEvent e, uint32_t w, uint32_t h)>;
	
	class Window {
	public:
		static Window* CreateGameWindow(const WindowInitialisation& init);

		static void DestroyGameWindow() {
			delete window;
			window = nullptr;
		}

		bool		IsMinimised() const { return minimised;	 }

		bool		IsDragged() const { return dragged; }

		bool		UpdateWindow();

		bool		HasInitialised()	const { return init; }

		float		GetScreenAspect()	const {
			return (float)size.x / (float)size.y;
		}

		Vector2i		GetScreenSize()		const { return size; }
		Vector2i		GetScreenPosition()	const { return position; }

		const std::string&  GetTitle()   const { return windowTitle; }
		void				SetTitle(const std::string& title) {
			windowTitle = title;
			UpdateTitle();
		};

		virtual void	LockMouseToWindow(bool lock) {};
		virtual void	ShowOSPointer(bool show) {};

		virtual void	SetWindowPosition(int x, int y) {};
		virtual void	SetFullScreen(bool state) {};
		virtual void	SetConsolePosition(int x, int y) {};
		virtual void	ShowConsole(bool state) {};

		static const Keyboard*	 GetKeyboard() { return keyboard; }
		static const Mouse*		 GetMouse() { return mouse; }
		static const GameTimer&	 GetTimer() { return timer; }

		static Window*	const GetWindow() { return window; }

		void SetWindowEventHandler(const WindowEventHandler& e) {
			eventHandler = e;
		}
	protected:
		Window();
		virtual ~Window();

		virtual void UpdateTitle() {}

		virtual bool InternalUpdate() = 0;

		WindowEventHandler eventHandler;

		bool				dragged		= false;
		bool				minimised	= false;
		bool				init		= false;
		Vector2i			position;
		Vector2i			size;
		Vector2i			defaultSize;

		std::string			windowTitle;

		static Window*		window;
		static Keyboard*	keyboard;
		static Mouse*		mouse;

		static GameTimer	timer;
	};
}