/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include <iostream>

namespace NCL::Maths {
	class Vector3;
	class Vector4;
	class Matrix3;
	class Quaternion;

	class Matrix4 {
	public:
		float	array[4][4];
	public:
		Matrix4(void);
		Matrix4(float elements[16]);
		Matrix4(const Matrix3& m3);
		Matrix4(const Quaternion& quat);
		~Matrix4(void) = default;

		//Set all matrix values to zero
		void	ToZero();

		//Gets the OpenGL position vector (floats 12,13, and 14)
		Vector3 GetPositionVector() const;
		//Sets the OpenGL position vector (floats 12,13, and 14)
		void	SetPositionVector(const Vector3 &in);

		//Gets the scale vector (floats 1,5, and 10)
		Vector3 GetDiagonal() const;
		//Sets the scale vector (floats 1,5, and 10)
		void	SetDiagonal(const Vector3& in);

		//Creates a rotation matrix that rotates by 'degrees' around the 'axis'
		//Analogous to glRotatef
		static Matrix4 Rotation(float degrees, const Vector3& axis);

		//Creates a scaling matrix (puts the 'scale' vector down the diagonal)
		//Analogous to glScalef
		static Matrix4 Scale(const Vector3& scale);

		//Creates a translation matrix (identity, with 'translation' vector at
		//floats 12, 13, and 14. Analogous to glTranslatef
		static Matrix4 Translation(const Vector3& translation);

		//Creates a perspective matrix, with 'znear' and 'zfar' as the near and 
		//far planes, using 'aspect' and 'fov' as the aspect ratio and vertical
		//field of vision, respectively.
		static Matrix4 Perspective(float znear, float zfar, float aspect, float fov);

		//Creates an orthographic matrix with 'znear' and 'zfar' as the near and 
		//far planes, and so on. Descriptive variable names are a good thing!
		static Matrix4 Orthographic(float left, float right, float bottom, float top, float near, float far); 

		//Builds a view matrix suitable for sending straight to the vertex shader.
		//Puts the camera at 'from', with 'lookingAt' centered on the screen, with
		//'up' as the...up axis (pointing towards the top of the screen)
		static Matrix4 BuildViewMatrix(const Vector3& from, const Vector3& lookingAt, const Vector3& up);

		void    Invert();
		Matrix4 Inverse() const;

		void	Transpose();
		Matrix4 Transposed() const;


		Vector4 GetRow(unsigned int row) const;
		Vector4 GetColumn(unsigned int column) const;

		//Multiplies 'this' matrix by matrix 'a'. Performs the multiplication in 'OpenGL' order (ie, backwards)
		inline Matrix4 operator*(const Matrix4& a) const {
			Matrix4 out;
			for (unsigned int c = 0; c < 4; ++c) {
				for (unsigned int r = 0; r < 4; ++r) {
					out.array[c][r] = 0.0f;
					for (unsigned int i = 0; i < 4; ++i) {
						out.array[c][r] += this->array[i][r] * a.array[c][i];
					}
				}
			}
			return out;
		}

		Vector3 operator*(const Vector3& v) const;
		Vector4 operator*(const Vector4& v) const;

		//Handy string output for the matrix. Can get a bit messy, but better than nothing!
		inline friend std::ostream& operator<<(std::ostream& o, const Matrix4& m) {
			o << "Mat4(";
			o << "\t"   << m.array[0][0] << "," << m.array[0][1] << "," << m.array[0][2] << "," << m.array[0][3] << "\n";
			o << "\t\t" << m.array[1][0] << "," << m.array[1][1] << "," << m.array[1][2] << "," << m.array[1][3] << "\n";
			o << "\t\t" << m.array[2][0] << "," << m.array[2][1] << "," << m.array[2][2] << "," << m.array[2][3] << "\n";
			o << "\t\t" << m.array[3][0] << "," << m.array[3][1] << "," << m.array[3][2] << "," << m.array[3][3] << " )\n";
			return o;
		}
	};
}