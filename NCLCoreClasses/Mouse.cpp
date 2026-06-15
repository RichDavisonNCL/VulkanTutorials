#include "Mouse.h"

using namespace NCL;

Mouse::Mouse() {
	memset(buttons		, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(holdButtons	, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(doubleClicks	, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(lastClickTime, 0, sizeof(float) * MouseButtons::MAX_VAL);

	isAwake		= true;
	lastWheel	= 0;
	frameWheel	= 0;
	sensitivity = 0.07f;
	clickLimit	= 200.0f;
}

void Mouse::UpdateFrameState(float msec) {
	memcpy(holdButtons, buttons, sizeof(bool)  * MouseButtons::MAX_VAL);

	//We sneak this in here, too. Resets how much the mouse has moved since last update
	relativePosition = Vector2();
	//And the same for the mouse wheel
	frameWheel = 0;

	for (int i = 0; i < MouseButtons::MAX_VAL; ++i) {
		if (lastClickTime[i] > 0) {
			lastClickTime[i] -= msec;
			if (lastClickTime[i] <= 0.0f) {
				doubleClicks[i] = false;
				lastClickTime[i] = 0.0f;
			}
		}
	}
}

void	Mouse::Sleep() {
	isAwake = false; 
	memset(buttons		, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(holdButtons	, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(doubleClicks	, 0, sizeof(bool)  * MouseButtons::MAX_VAL);
	memset(lastClickTime, 0, sizeof(float) * MouseButtons::MAX_VAL);
	lastWheel  = 0;
	frameWheel = 0;
}

void	Mouse::Wake() {
	isAwake = true; 
}

void	Mouse::SetAbsolutePosition(const Vector2& pos) {
	absolutePosition = pos;
}

void	Mouse::SetAbsolutePositionBounds(const Vector2i& bounds) {
	absolutePositionBounds = bounds;
}