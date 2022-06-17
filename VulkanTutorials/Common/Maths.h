/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once

namespace NCL::Maths {
	class Vector2;
	class Vector3;

	class Vector2i {
	public:
		int array[2];

		Vector2i() {
			array[0] = 0;
			array[1] = 0;
		}
		inline int operator[](int i) const {
			return array[i];
		}

		inline int& operator[](int i) {
			return array[i];
		}
	};

	class Vector3i {
	public:
		int array[3];

		Vector3i() {
			array[0] = 0;
			array[1] = 0;
			array[2] = 0;
		}
		inline int operator[](int i) const {
			return array[i];
		}

		inline int& operator[](int i) {
			return array[i];
		}
	};

	class Vector4i {
	public:
		int array[4];

		Vector4i() {
			array[0] = 0;
			array[1] = 0;
			array[2] = 0;
			array[3] = 0;
		}
		inline int operator[](int i) const {
			return array[i];
		}

		inline int& operator[](int i) {
			return array[i];
		}
	};

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
	float Areaof2DTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);

	float AreaofTri3D(const Vector3 &a, const Vector3 &b, const Vector3 & c);
}