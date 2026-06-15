/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once

class Controller	{
public:
	Controller() {

	}
	virtual ~Controller() {

	}

	virtual float	GetAxis(uint32_t axis) const = 0;
	virtual float	GetButtonAnalogue(uint32_t button) const = 0;
	virtual bool	GetButton(uint32_t button) const = 0;


	virtual float	GetNamedAxis(const std::string& axis) const;
	virtual float	GetNamedButtonAnalogue(const std::string& button) const;
	virtual bool	GetNamedButton(const std::string& button) const;

	void MapAxis(uint32_t axis, const std::string& name);
	void MapButton(uint32_t axis, const std::string& name);
	void MapButtonAnalogue(uint32_t axis, const std::string& name);

	virtual void Update(float dt = 0.0f) {}

protected:

	std::map<std::string, uint32_t> buttonMappings;
	std::map<std::string, uint32_t> analogueMappings;
	std::map<std::string, uint32_t> axisMappings;
};