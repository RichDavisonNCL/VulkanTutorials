/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Vector.h"

namespace NCL::Maths {
	//It's pi(ish)...
	static const float		PI = 3.14159265358979323846f;

	//It's pi...divided by 360.0f!
	static const float		PI_OVER_360 = PI / 360.0f;

	//Radians to degrees
	inline float RadiansToDegrees(float rads) {
		return rads * 180.0f / PI;
	};

	//Degrees to radians
	inline float DegreesToRadians(float degs) {
		return degs * PI / 180.0f;
	};

	float RandomValue(float min, float max);

	void ScreenBoxOfTri(const Vector3& v0, const Vector3& v1, const Vector3& v2, Vector2& topLeft, Vector2& bottomRight);

	int ScreenAreaOfTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);
	float SignedAreaof2DTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);

	float AreaofTri3D(const Vector3 &a, const Vector3 &b, const Vector3 & c);

	template<typename T>
	T EvaluateCubicBezier(float t, const T& p0, const T& p1, const T& p2, const T& p3) {
		T p0p1p2 = EvaluateQuadtraticBezier(t, p0, p1, p2);
		T p1p2p3 = EvaluateQuadtraticBezier(t, p1, p2, p3);

		return T((p0p1p2 * (1.0f - t)) + (p1p2p3 * (t)));
	}

	template<typename T>
	T EvaluateQuadraticBezier(float t, const T& p0, const T& p1, const T& p2) {
		T p0p1 = (p0 * (1.0f - t)) + (p1 * t);
		T p1p2 = (p1 * (1.0f - t)) + (p2 * t);

		return (p0p1 * (1.0f - t)) + (p1p2 * t);
	}


}