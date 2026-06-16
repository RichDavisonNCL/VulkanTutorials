/** @file FrustumCulling.h
 * Pure-math frustum plane extraction (Gribb-Hartmann) and AABB-intersection.
 * Shared between CPU culling paths and tests. No Vulkan dependency.
 * Types fully qualified (NCL::Matrix4 etc.) to avoid include-order issues.
 */
#pragma once
#include "Matrix.h"
#include "Vector.h"

/** Extracts 6 frustum planes (inward-pointing normals) from VP matrix.
 *  Gribb-Hartmann: each plane = sum/difference of VP matrix rows. */
inline void NCL_ExtractFrustumPlanes(const NCL::Matrix4& vp, NCL::Vector4 planes[6]) {
	const auto& m = vp.array;
	planes[0] = NCL::Vector4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
	planes[1] = NCL::Vector4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
	planes[2] = NCL::Vector4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
	planes[3] = NCL::Vector4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
	planes[4] = NCL::Vector4(m[0][2],           m[1][2],           m[2][2],           m[3][2]);
	planes[5] = NCL::Vector4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
	for (int i = 0; i < 6; ++i) {
		float len = std::sqrt(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
		if (len > 0.0f) planes[i] = planes[i] / len;
	}
}

/** Conservative AABB-frustum intersection test.
 *  Returns true if AABB is at least partially inside. */
inline bool NCL_AABBInFrustum(const NCL::Vector4 planes[6],
                              float minX, float minY, float minZ,
                              float maxX, float maxY, float maxZ) {
	for (int i = 0; i < 6; ++i) {
		NCL::Vector3 p(planes[i].x > 0 ? maxX : minX,
		               planes[i].y > 0 ? maxY : minY,
		               planes[i].z > 0 ? maxZ : minZ);
		if (NCL::Vector::Dot(NCL::Vector3(planes[i].x, planes[i].y, planes[i].z), p) + planes[i].w < 0)
			return false;
	}
	return true;
}
