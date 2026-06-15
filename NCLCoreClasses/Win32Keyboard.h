/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#ifdef _WIN32
#include "Keyboard.h"
#include "Win32Window.h"

namespace NCL::Win32Code {
	class Win32Keyboard : public Keyboard {
	public:
		friend class Win32Window;

	protected:
		Win32Keyboard(HWND &hwnd);
		virtual ~Win32Keyboard(void) {
		}

		virtual void UpdateRAW(RAWINPUT* raw);
		RAWINPUTDEVICE	rid;			//Windows OS hook 
	};
}
#endif //_WIN32