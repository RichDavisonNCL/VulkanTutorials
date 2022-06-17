/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#ifdef _WIN32
#include "Mouse.h"
#include "Win32Window.h"

namespace NCL::Win32Code {
	class Win32Mouse : public NCL::Mouse {
	public:
		friend class Win32Window;

	protected:
		Win32Mouse(HWND &hwnd);
		virtual ~Win32Mouse(void) {}

		void UpdateWindowPosition(const Vector2& newPos) {
			windowPosition = newPos;
		}

		virtual void	UpdateRAW(RAWINPUT* raw);
		RAWINPUTDEVICE	rid;			//Windows OS hook 

		bool		setAbsolute;
	};
}
#endif //_WIN32