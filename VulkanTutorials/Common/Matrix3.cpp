/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#include "Matrix3.h"
#include "Matrix2.h"
#include "Matrix4.h"

#include "Maths.h"
#include "Vector3.h"
#include "Quaternion.h"
#include <assert.h>

using namespace NCL;
using namespace NCL::Maths;

Matrix3::Matrix3(void)	{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++i) {
			array[i][j] = 0.0f;
		}
	}
	array[0][0] = 1.0f;
	array[1][1] = 1.0f;
	array[2][2] = 1.0f;
}

Matrix3::Matrix3(float elements[9]) {
	array[0][0]  = elements[0];
	array[0][1]  = elements[1];
	array[0][2]  = elements[2];

	array[1][0]  = elements[4];
	array[1][1]  = elements[5];
	array[2][2]  = elements[6];

	array[2][0]  = elements[8];
	array[2][1]  = elements[9];
	array[2][2]  = elements[10];
}

Matrix3::Matrix3(const Matrix4 &m4) {
	array[0][0] = m4.array[0][0];
	array[0][1] = m4.array[0][1];
	array[0][2] = m4.array[0][2];

	array[1][0] = m4.array[1][0];
	array[1][1] = m4.array[1][1];
	array[2][2] = m4.array[2][2];

	array[2][0] = m4.array[2][0];
	array[2][1] = m4.array[2][1];
	array[2][2] = m4.array[2][2];
}

Matrix3::Matrix3(const Matrix2 &m2) {
	array[0][0] = m2.array[0][0];
	array[0][1] = m2.array[0][1];
	array[0][2] = 0;

	array[1][0] = m2.array[1][0];
	array[1][1] = m2.array[1][1];
	array[1][2] = 0.0f;

	array[2][0] = 0.0f;
	array[2][1] = 0.0f;
	array[2][2] = 1.0f;
}

Matrix3::Matrix3(const Quaternion &quat) {
	float yy = quat.y * quat.y;
	float zz = quat.z * quat.z;
	float xy = quat.x * quat.y;
	float zw = quat.z * quat.w;
	float xz = quat.x * quat.z;
	float yw = quat.y * quat.w;
	float xx = quat.x * quat.x;
	float yz = quat.y * quat.z;
	float xw = quat.x * quat.w;

	array[0][0] = 1 - 2 * yy - 2 * zz;
	array[0][1] = 2 * xy + 2 * zw;
	array[0][2] = 2 * xz - 2 * yw;

	array[1][0] = 2 * xy - 2 * zw;
	array[1][1] = 1 - 2 * xx - 2 * zz;
	array[1][2] = 2 * yz + 2 * xw;

	array[2][0] = 2 * xz + 2 * yw;
	array[2][1] = 2 * yz - 2 * xw;
	array[2][2] = 1 - 2 * xx - 2 * yy;
}

Matrix3 Matrix3::Rotation(float degrees, const Vector3 &inaxis)	 {
	Matrix3 m;

	Vector3 axis = inaxis;

	axis.Normalise();

	float c = cos(Maths::DegreesToRadians(degrees));
	float s = sin(Maths::DegreesToRadians(degrees));

	m.array[0][0]  = (axis.x * axis.x) * (1.0f - c) + c;
	m.array[0][1]  = (axis.y * axis.x) * (1.0f - c) + (axis.z * s);
	m.array[0][2]  = (axis.z * axis.x) * (1.0f - c) - (axis.y * s);

	m.array[1][0]  = (axis.x * axis.y) * (1.0f - c) - (axis.z * s);
	m.array[1][1]  = (axis.y * axis.y) * (1.0f - c) + c;
	m.array[1][2]  = (axis.z * axis.y) * (1.0f - c) + (axis.x * s);

	m.array[2][0]  = (axis.x * axis.z) * (1.0f - c) + (axis.y * s);
	m.array[2][1]  = (axis.y * axis.z) * (1.0f - c) - (axis.x * s);
	m.array[2][2]  = (axis.z * axis.z) * (1.0f - c) + c;

	return m;
}

Matrix3 Matrix3::Scale( const Vector3 &scale )	{
	Matrix3 m;

	m.array[0][0]  = scale.x;
	m.array[1][1]  = scale.y;
	m.array[2][2]  = scale.z;	

	return m;
}

void	Matrix3::ToZero()	{
	for(int i = 0; i < 9; ++i) {
		array[0][0] = 0.0f;
	}
}

//http://staff.city.ac.uk/~sbbh653/publications/euler.pdf
Vector3 Matrix3::ToEuler() const {
	//float h = (float)RadiansToDegrees(atan2(-values[6], values[0]));
	//float b = (float)RadiansToDegrees(atan2(-values[5], values[4]));
	//float a = (float)RadiansToDegrees(asin(values[3]));

	//return Vector3(a, h, b);

	//psi  = x;
	//theta = y;
	//phi = z



	float testVal = abs(array[0][2]) + 0.00001f;

	if (testVal < 1.0f) {
		float theta1 = -asin(array[0][2]);
		float theta2 = Maths::PI - theta1;

		float cost1 = cos(theta1);
		//float cost2 = cos(theta2);

		float psi1 = Maths::RadiansToDegrees(atan2(array[1][2] / cost1, array[2][2] / cost1));
		//float psi2 = Maths::RadiansToDegrees(atan2(array[1][2] / cost2, array[2][2] / cost2));

		float phi1 = Maths::RadiansToDegrees(atan2(array[0][1] / cost1, array[0][0] / cost1));
		//float phi2 = Maths::RadiansToDegrees(atan2(array[0][1] / cost2, array[0][0] / cost2));

		theta1 = Maths::RadiansToDegrees(theta1);
		//theta2 = Maths::RadiansToDegrees(theta2);

		return Vector3(psi1, theta1, phi1);
	}
	else {
		float phi	= 0.0f;	//x


		float theta = 0.0f;	//y
		float psi	= 0.0f;	//z

		float delta = atan2(array[1][0], array[2][0]);

		if (array[0][2] < 0.0f) {
			theta = Maths::PI / 2.0f;
			psi = phi + delta;
		}
		else {
			theta = -Maths::PI / 2.0f;
			psi = phi + delta;
		}

		return Vector3(Maths::RadiansToDegrees(psi), Maths::RadiansToDegrees(theta), Maths::RadiansToDegrees(phi));
	}
}

Matrix3 Matrix3::FromEuler(const Vector3 &euler) {
	Matrix3 m;

	float heading	= Maths::DegreesToRadians(euler.y);
	float attitude	= Maths::DegreesToRadians(euler.x);
	float bank		= Maths::DegreesToRadians(euler.z);

	float ch = cos(heading);
	float sh = sin(heading);
	float ca = cos(attitude);
	float sa = sin(attitude);
	float cb = cos(bank);
	float sb = sin(bank);

	m.array[0][0] = ch * ca;
	m.array[1][0] = sh*sb - ch*sa*cb;
	m.array[2][0] = ch*sa*sb + sh*cb;
	m.array[0][1] = sa;
	m.array[1][1] = ca*cb;
	m.array[2][1] = -ca*sb;
	m.array[0][2] = -sh*ca;
	m.array[1][2] = sh*sa*cb + ch*sb;
	m.array[2][2] = -sh*sa*sb + ch*cb;

	return m;
}

Vector3 Matrix3::GetRow(unsigned int row) const {
	assert(row < 3);
	return Vector3(
		array[0][row],
		array[1][row],
		array[2][row]
	);
}

Matrix3& Matrix3::SetRow(unsigned int row, const Vector3 &val) {
	assert(row < 3);
	array[0][row] = val.x;
	array[1][row] = val.y;
	array[2][row] = val.z;
	return *this;
}

Vector3 Matrix3::GetColumn(unsigned int column) const {
	assert(column < 3);
	return Vector3(
		array[column][0],
		array[column][1],
		array[column][2]
	);
}

Matrix3& Matrix3::SetColumn(unsigned int column, const Vector3 &val) {
	assert(column < 3);
	array[column][0] = val.x;
	array[column][1] = val.y;
	array[column][2] = val.z;
	return *this;
}

Vector3 Matrix3::GetDiagonal() const {
	return Vector3(array[0][0], array[1][1], array[2][2]);
}

void	Matrix3::SetDiagonal(const Vector3 &in) {
	array[0][0] = in.x;
	array[1][1] = in.y;
	array[2][2] = in.z;
}

Vector3 Matrix3::operator*(const Vector3 &v) const {
	Vector3 vec;

	vec.x = v.x*array[0][0] + v.y*array[1][0] + v.z*array[2][0];
	vec.y = v.x*array[0][1] + v.y*array[1][1] + v.z*array[2][1];
	vec.z = v.x*array[0][2] + v.y*array[1][2] + v.z*array[2][2];

	return vec;
};