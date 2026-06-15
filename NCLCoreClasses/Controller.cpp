/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#include "Controller.h"

void Controller::MapAxis(uint32_t axis, const std::string& name)  {
	axisMappings.insert({name, axis});
}

void Controller::MapButton(uint32_t axis, const std::string& name) {
	buttonMappings.insert({ name, axis });
}

void Controller::MapButtonAnalogue(uint32_t axis, const std::string& name) {
	analogueMappings.insert({ name, axis });
}

float	Controller::GetNamedAxis(const std::string& axis) const {
	auto a = axisMappings.find(axis);
	if (a != axisMappings.end()) {
		return GetAxis(a->second);
	}
	return 0.0f;
}

float	Controller::GetNamedButtonAnalogue(const std::string& button) const {
	auto a = analogueMappings.find(button);
	if (a != analogueMappings.end()) {
		return GetButtonAnalogue(a->second);
	}
	return 0.0f;
}

bool	Controller::GetNamedButton(const std::string& button) const {
	auto a = buttonMappings.find(button);
	if (a != buttonMappings.end()) {
		return GetButton(a->second);
	}
	return false;
}