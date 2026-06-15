/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Controller.h"

namespace NCL {
	using namespace NCL::Maths;
	class Camera {
	public:
		Camera(void) {
			pitch		= 0.0f;
			yaw			= 0.0f;

			nearPlane	= 1.0f;
			farPlane	= 1000.0f;
			speed		= 100.0f;
		};

		Camera(float pitch, float yaw, const Vector3& position) : Camera() {
			this->pitch		= pitch;
			this->yaw		= yaw;
			this->position	= position;
		}

		~Camera(void) = default;

		virtual void UpdateCamera(float dt);

		float GetNearPlane() const {
			return nearPlane;
		}

		float GetFarPlane() const {
			return farPlane;
		}

		Camera& SetNearPlane(float val) {
			nearPlane = val;
			return *this;
		}
		
		Camera& SetFarPlane(float val) {
			farPlane = val;
			return *this;
		}

		Camera& SetController(const Controller& c) {
			activeController = &c;
			return *this;
		}

		//Builds a view matrix for the current camera variables, suitable for sending straight
		//to a vertex shader (i.e it's already an 'inverse camera matrix').
		Matrix4 BuildViewMatrix() const;

		virtual Matrix4 BuildProjectionMatrix(float aspectRatio = 1.0f) const = 0;

		//Gets position in world space
		Vector3 GetPosition() const { return position; }
		//Sets position in world space
		Camera&	SetPosition(const Vector3& val) { position = val;  return *this; }

		//Gets yaw, in degrees
		float	GetYaw()   const	{ return yaw; }
		//Sets yaw, in degrees
		Camera&	SetYaw(float y)		{ yaw = y;  return *this; }

		//Gets pitch, in degrees
		float	GetPitch() const	{ return pitch; }
		//Sets pitch, in degrees
		Camera& SetPitch(float p)	{ pitch = p; return *this; }

		float	GetSpeed() const	{ return speed; }
		Camera& SetSpeed(float s)	{ speed = s; return *this; }

	protected:
		float	nearPlane;
		float	farPlane;

		float	yaw;
		float	pitch;
		Vector3 position;
		float	speed;

		const Controller* activeController = nullptr;
	};

	class OrhographicCamera : public Camera {
	public:
		OrhographicCamera() {
			left	= 0;
			right	= 0;
			top		= 0;
			bottom	= 0;
		}
		~OrhographicCamera() = default;

		Matrix4 BuildProjectionMatrix(float aspectRatio = 1.0f) const override;

	protected:
		float	left;
		float	right;
		float	top;
		float	bottom;
	};

	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera() {
			fov = 45.0f;
		}
		~PerspectiveCamera() = default;

		float GetFieldOfVision() const {
			return fov;
		}

		PerspectiveCamera& SetFieldOfVision(float val) {
			fov = val;
			return *this;
		}

		Matrix4 BuildProjectionMatrix(float aspectRatio = 1.0f) const override;

	protected:
		float	fov;
	};
}