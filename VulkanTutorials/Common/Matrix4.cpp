/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#include "Matrix4.h"
#include "Matrix3.h"
#include "Maths.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"

using namespace NCL;
using namespace NCL::Maths;
Matrix4::Matrix4(void)	{
	ToZero();
	array[0][0] = 1.0f;
	array[1][1] = 1.0f;
	array[2][2] = 1.0f;
	array[3][3] = 1.0f;
}

Matrix4::Matrix4( float elements[16] )	{
	memcpy(this->array,elements,16*sizeof(float));
}

Matrix4::Matrix4(const Matrix3& m3) {
	array[0][0] = m3.array[0][0];
	array[0][1] = m3.array[0][1];
	array[0][2] = m3.array[0][2];

	array[1][0] = m3.array[1][0];
	array[1][1] = m3.array[1][1];
	array[1][2] = m3.array[1][2];

	array[2][0]  = m3.array[2][0];
	array[2][1]  = m3.array[2][1];
	array[2][2]  = m3.array[2][2];

	array[3][0] = 0.0f;
	array[3][1] = 0.0f;
	array[3][2] = 0.0f;
	array[3][3] = 1.0f;
}

Matrix4::Matrix4(const Quaternion& quat) : Matrix4() {
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
	array[2][1]  = 2 * yz - 2 * xw;
	array[2][2]  = 1 - 2 * xx - 2 * yy;
}

void Matrix4::ToZero()	{
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			array[i][j] = 0.0f;
		}
	}
}

Vector3 Matrix4::GetPositionVector() const{
	return Vector3(array[3][0] ,array[3][1] ,array[3][2] );
}

void	Matrix4::SetPositionVector(const Vector3& in) {
	array[3][0] = in.x;
	array[3][1] = in.y;
	array[3][2] = in.z;		
}

Vector3 Matrix4::GetDiagonal() const{
	return Vector3(array[0][0],array[1][1],array[2][2]);
}

void	Matrix4::SetDiagonal(const Vector3 &in) {
	array[0][0]  = in.x;
	array[1][1]  = in.y;
	array[2][2]  = in.z;		
}

Matrix4 Matrix4::Perspective(float znear, float zfar, float aspect, float fov) {
	Matrix4 m;

	const float h = 1.0f / tan(fov*Maths::PI_OVER_360);
	float neg_depth = znear-zfar;

	m.array[0][0]		= h / aspect;
	m.array[1][1]		= h;
	m.array[2][2] 	= (zfar + znear)/neg_depth;
	m.array[2][3] 	= -1.0f;
	m.array[3][2] 	= 2.0f*(znear*zfar)/neg_depth;
	m.array[3][3] 	= 0.0f;

	return m;
}

//http://www.opengl.org/sdk/docs/man/xhtml/glOrtho.xml
Matrix4 Matrix4::Orthographic(float left, float right, float bottom, float top, float near, float far)	{
	Matrix4 m;

	m.array[0][0]	= 2.0f / (right-left);
	m.array[1][1]	= 2.0f / (top-bottom);
	m.array[2][2] 	= -2.0f / (far-near);

	m.array[3][0]   = -(right+left)/(right-left);
	m.array[3][1]   = -(top+bottom)/(top-bottom);
	m.array[3][2]   = -(far + near)/(far - near);
	m.array[3][3]   = 1.0f;

	return m;
}

Matrix4 Matrix4::BuildViewMatrix(const Vector3& from, const Vector3& lookingAt, const Vector3& up)	{
	Matrix4 r;
	r.SetPositionVector(Vector3(-from.x,-from.y,-from.z));

	Matrix4 m;

	Vector3 f = (lookingAt - from);
	f.Normalise();

	Vector3 s = Vector3::Cross(f,up).Normalised();
	Vector3 u = Vector3::Cross(s,f).Normalised();

	m.array[0][0] = s.x;
	m.array[1][0] = s.y;
	m.array[2][0] = s.z;

	m.array[0][1] = u.x;
	m.array[1][1] = u.y;
	m.array[2][1]  = u.z;

	m.array[0][2]  = -f.x;
	m.array[1][2]  = -f.y;
	m.array[2][2]  = -f.z;

	return m*r;
}

Matrix4 Matrix4::Rotation(float degrees, const Vector3 &inaxis)	 {
	Matrix4 m;

	Vector3 axis = inaxis;

	axis.Normalise();

	float c = cos((float)Maths::DegreesToRadians(degrees));
	float s = sin((float)Maths::DegreesToRadians(degrees));

	m.array[0][0]  = (axis.x * axis.x) * (1.0f - c) + c;
	m.array[0][1]  = (axis.y * axis.x) * (1.0f - c) + (axis.z * s);
	m.array[0][2]  = (axis.z * axis.x) * (1.0f - c) - (axis.y * s);

	m.array[1][0]  = (axis.x * axis.y) * (1.0f - c) - (axis.z * s);
	m.array[1][1]  = (axis.y * axis.y) * (1.0f - c) + c;
	m.array[1][2]  = (axis.z * axis.y) * (1.0f - c) + (axis.x * s);

	m.array[2][0]  = (axis.x * axis.z) * (1.0f - c) + (axis.y * s);
	m.array[2][1]   = (axis.y * axis.z) * (1.0f - c) - (axis.x * s);
	m.array[2][2]  = (axis.z * axis.z) * (1.0f - c) + c;

	return m;
}

Matrix4 Matrix4::Scale( const Vector3 &scale )	{
	Matrix4 m;
	m.array[0][0] = scale.x;
	m.array[1][1] = scale.y;
	m.array[2][2] = scale.z;
	return m;
}

Matrix4 Matrix4::Translation( const Vector3 &translation )	{
	Matrix4 m;
	m.array[3][0] = translation.x;
	m.array[3][1] = translation.y;
	m.array[3][2] = translation.z;	
	return m;
}

//Yoinked from the Open Source Doom 3 release - all credit goes to id software!
void    Matrix4::Invert() {
	float det, invDet;

	// 2x2 sub-determinants required to calculate 4x4 determinant
	float det2_01_01 = array[0][0] * array[1][1] - array[0][1] * array[1][0];
	float det2_01_02 = array[0][0] * array[1][2] - array[0][2] * array[1][0];
	float det2_01_03 = array[0][0] * array[1][3] - array[0][3] * array[1][0];
	float det2_01_12 = array[0][1] * array[1][2] - array[0][2] * array[1][1];
	float det2_01_13 = array[0][1] * array[1][3] - array[0][3] * array[1][1];
	float det2_01_23 = array[0][2] * array[1][3] - array[0][3] * array[1][2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	float det3_201_012 = array[2][0] * det2_01_12 - array[2][1]  * det2_01_02 + array[2][2]  * det2_01_01;
	float det3_201_013 = array[2][0] * det2_01_13 - array[2][1]  * det2_01_03 + array[2][3]  * det2_01_01;
	float det3_201_023 = array[2][0] * det2_01_23 - array[2][2]  * det2_01_03 + array[2][3]  * det2_01_02;
	float det3_201_123 = array[2][1]  * det2_01_23 - array[2][2]  * det2_01_13 + array[2][3]  * det2_01_12;

	det = (-det3_201_123 * array[3][0]  + det3_201_023 * array[3][1]  - det3_201_013 * array[3][2]  + det3_201_012 * array[3][3] );

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_03_01 = array[0][0] * array[3][1]  - array[0][1] * array[3][0] ;
	float det2_03_02 = array[0][0] * array[3][2]  - array[0][2] * array[3][0] ;
	float det2_03_03 = array[0][0] * array[3][3]  - array[0][3] * array[3][0] ;
	float det2_03_12 = array[0][1] * array[3][2]  - array[0][2] * array[3][1] ;
	float det2_03_13 = array[0][1] * array[3][3]  - array[0][3] * array[3][1] ;
	float det2_03_23 = array[0][2] * array[3][3]  - array[0][3] * array[3][2] ;

	float det2_13_01 = array[1][0] * array[3][1]  - array[1][1] * array[3][0] ;
	float det2_13_02 = array[1][0] * array[3][2]  - array[1][2] * array[3][0] ;
	float det2_13_03 = array[1][0] * array[3][3]  - array[1][3] * array[3][0] ;
	float det2_13_12 = array[1][1] * array[3][2]  - array[1][2] * array[3][1] ;
	float det2_13_13 = array[1][1] * array[3][3]  - array[1][3] * array[3][1] ;
	float det2_13_23 = array[1][2] * array[3][3]  - array[1][3] * array[3][2] ;

	// remaining 3x3 sub-determinants
	float det3_203_012 = array[2][0] * det2_03_12 - array[2][1]  * det2_03_02 + array[2][2]  * det2_03_01;
	float det3_203_013 = array[2][0] * det2_03_13 - array[2][1]  * det2_03_03 + array[2][3]  * det2_03_01;
	float det3_203_023 = array[2][0] * det2_03_23 - array[2][2]  * det2_03_03 + array[2][3]  * det2_03_02;
	float det3_203_123 = array[2][1]  * det2_03_23 - array[2][2]  * det2_03_13 + array[2][3]  * det2_03_12;

	float det3_213_012 = array[2][0] * det2_13_12 - array[2][1]  * det2_13_02 + array[2][2]  * det2_13_01;
	float det3_213_013 = array[2][0] * det2_13_13 - array[2][1]  * det2_13_03 + array[2][3]  * det2_13_01;
	float det3_213_023 = array[2][0] * det2_13_23 - array[2][2]  * det2_13_03 + array[2][3]  * det2_13_02;
	float det3_213_123 = array[2][1]  * det2_13_23 - array[2][2]  * det2_13_13 + array[2][3]  * det2_13_12;

	float det3_301_012 = array[3][0]  * det2_01_12 - array[3][1]  * det2_01_02 + array[3][2]  * det2_01_01;
	float det3_301_013 = array[3][0]  * det2_01_13 - array[3][1]  * det2_01_03 + array[3][3]  * det2_01_01;
	float det3_301_023 = array[3][0]  * det2_01_23 - array[3][2]  * det2_01_03 + array[3][3]  * det2_01_02;
	float det3_301_123 = array[3][1]  * det2_01_23 - array[3][2]  * det2_01_13 + array[3][3]  * det2_01_12;

	array[0][0] = -det3_213_123 * invDet;
	array[1][0] = +det3_213_023 * invDet;
	array[2][0] = -det3_213_013 * invDet;
	array[3][0]  = +det3_213_012 * invDet;

	array[0][1] = +det3_203_123 * invDet;
	array[1][1] = -det3_203_023 * invDet;
	array[2][1]  = +det3_203_013 * invDet;
	array[3][1]  = -det3_203_012 * invDet;

	array[0][2] = +det3_301_123 * invDet;
	array[1][2] = -det3_301_023 * invDet;
	array[2][2]  = +det3_301_013 * invDet;
	array[3][2]  = -det3_301_012 * invDet;

	array[0][3] = -det3_201_123 * invDet;
	array[1][3] = +det3_201_023 * invDet;
	array[2][3]  = -det3_201_013 * invDet;
	array[3][3]  = +det3_201_012 * invDet;
}

Matrix4 Matrix4::Inverse()	const {
	Matrix4 temp(*this);
	temp.Invert();
	return temp;
}

void	Matrix4::Transpose() {
	for (int i = 0; i < 4; ++i) {
		for (int j =i+1; j < 4; ++j) {
			float temp = array[i][j];
			array[i][j] = array[j][i];
			array[j][i] = temp;
		}
	}
}

Matrix4 Matrix4::Transposed() const {
	Matrix4 temp(*this);
	temp.Transpose();
	return temp;
}

Vector4 Matrix4::GetRow(unsigned int row) const {
	Vector4 out(0, 0, 0, 1);
	if (row <= 3) {
		out.x = array[0][row];
		out.y = array[1][row];
		out.z = array[2][row];
	}
	return out;
}

Vector4 Matrix4::GetColumn(unsigned int column) const {
	Vector4 out(0, 0, 0, 1);
	if (column <= 3) {
		out.x = array[column][0];
		out.y = array[column][1];
		out.z = array[column][2];
	}

	return out;
}

Vector3 Matrix4::operator*(const Vector3 &v) const {
	Vector3 vec;

	float temp;

	vec.x = v.x*array[0][0] + v.y*array[1][0] + v.z*array[2][0] + array[3][0] ;
	vec.y = v.x*array[0][1] + v.y*array[1][1] + v.z*array[2][1]  + array[3][1] ;
	vec.z = v.x*array[0][2] + v.y*array[1][2] + v.z*array[2][2]  + array[3][2] ;

	temp = v.x*array[0][3] + v.y*array[1][3] + v.z*array[2][3]  + array[3][3] ;

	vec.x = vec.x / temp;
	vec.y = vec.y / temp;
	vec.z = vec.z / temp;

	return vec;
}

Vector4 Matrix4::operator*(const Vector4 &v) const {
	return Vector4(
		v.x*array[0][0] + v.y*array[1][0] + v.z*array[2][0] + v.w * array[3][0] ,
		v.x*array[0][1] + v.y*array[1][1] + v.z*array[2][1]  + v.w * array[3][1] ,
		v.x*array[0][2] + v.y*array[1][2] + v.z*array[2][2]  + v.w * array[3][2] ,
		v.x*array[0][3] + v.y*array[1][3] + v.z*array[2][3]  + v.w * array[3][3] 
	);
}
