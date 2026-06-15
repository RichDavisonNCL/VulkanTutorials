/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Plane.h"
#include "Matrix.h"

namespace NCL::Maths {
	class Frustum {
	public:
		Frustum(void);
		~Frustum(void) {};		

		static Frustum FromViewProjMatrix(const Matrix4& mat, float ndcNear = -1.0f, float ndcFar = 1.0f);

		bool SphereInsideFrustum(const Vector3& position, float radius) const {
			for (int p = 0; p < 6; ++p) {
				if (!planes[p].SphereInPlane(position, radius)) {
					return false;
				}
			}
			return true;
		}
	protected:
		Plane planes[6];
	};
}