/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#include "Frustum.h"
#include "Vector.h"
#include "Matrix.h"

using namespace NCL;
using namespace NCL::Maths;

Frustum::Frustum(void) {

};

Frustum Frustum::FromViewProjMatrix(const Matrix4& viewProj, float ndcNear, float ndcFar) {
	Frustum f;

	Matrix4 invMatrix		= Matrix::Inverse(viewProj);

	//Takes NDC coordinates, transforms them into into clip space using the inverse matrix
	Vector4 topLeftFar		= invMatrix * Vector4(-1.0f,  1.0f, ndcFar, 1.0f);
	Vector4 topRightFar		= invMatrix * Vector4( 1.0f,  1.0f, ndcFar, 1.0f);
	Vector4 bottomLeftFar	= invMatrix * Vector4(-1.0f, -1.0f, ndcFar, 1.0f);
	Vector4 bottomRightFar	= invMatrix * Vector4( 1.0f, -1.0f, ndcFar, 1.0f);

	Vector4 topLeftNear		= invMatrix * Vector4(-1.0f,  1.0f, ndcNear, 1.0f);
	Vector4 topRightNear	= invMatrix * Vector4( 1.0f,  1.0f, ndcNear, 1.0f);
	Vector4 bottomLeftNear	= invMatrix * Vector4(-1.0f, -1.0f, ndcNear, 1.0f);
	Vector4 bottomRightNear = invMatrix * Vector4( 1.0f, -1.0f, ndcNear, 1.0f);

	//To bring them fully into 'world' coodinates, we must divide them by their w component

	topLeftFar		/= topLeftFar.w;
	topRightFar		/= topRightFar.w;
	bottomLeftFar	/= bottomLeftFar.w;
	bottomRightFar	/= bottomRightFar.w;

	topLeftNear		/= topLeftNear.w;
	topRightNear	/= topRightNear.w;
	bottomLeftNear	/= bottomLeftNear.w;
	bottomRightNear /= bottomRightNear.w;

	//Now they are in world space, we can form planes from them, 3 at a time
	//Note that the order is important, to make sure that the positive half space of the plane
	//is facing 'in' to the frustum - a point positive to all 6 planes is inside the frustum
	
	f.planes[0] = Plane::PlaneFromTri(bottomLeftFar, topLeftFar, topLeftNear);		//left plane
	f.planes[1] = Plane::PlaneFromTri(topRightFar, bottomRightFar, topRightNear);	//right plane

	f.planes[2] = Plane::PlaneFromTri(topLeftFar, topRightFar, topRightNear);			//top plane
	f.planes[3] = Plane::PlaneFromTri(bottomRightFar, bottomLeftFar, bottomRightNear);	//bottom plane

	f.planes[4] = Plane::PlaneFromTri(topLeftNear, topRightNear, bottomRightNear);	//near plane
	f.planes[5] = Plane::PlaneFromTri(topRightFar, topLeftFar, bottomRightFar);		//far plane

	return f;
}