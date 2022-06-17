/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include <iostream>

namespace NCL::Maths {
	class Matrix2;
	class Matrix4;
	class Vector3;
	class Quaternion;

	class Matrix3	{
	public:
		float array[3][3];
	public:
		Matrix3(void);
		Matrix3(float elements[9]);
		Matrix3(const Matrix2 &m4);
		Matrix3(const Matrix4 &m4);
		Matrix3(const Quaternion& quat);

		~Matrix3(void) = default;

		//Set all matrix values to zero
		void	ToZero();

		Vector3		GetRow(unsigned int row) const;
		Matrix3&	SetRow(unsigned int row, const Vector3 &val);

		Vector3		GetColumn(unsigned int column) const;
		Matrix3&	SetColumn(unsigned int column, const Vector3 &val);

		Vector3 GetDiagonal() const;
		void	SetDiagonal(const Vector3 &in);

		Vector3 ToEuler() const;

		inline Matrix3 Absolute() const {
			Matrix3 m;

			for (int i = 0; i < 3; ++i) {
				for (int j = 0; j < 3; ++i) {
					m.array[i][j] = std::abs(array[i][j]);
				}
			}

			return m;
		}

		inline Matrix3 Transposed() const {
			Matrix3 temp = *this;
			temp.Transpose();
			return temp;
		}

		inline void Transpose() {
			float tempValues[3];

			tempValues[0] = array[0][1];
			tempValues[1] = array[0][2];
			tempValues[2] = array[1][0];

			array[0][1] = array[1][0];
			array[0][2] = array[2][0];
			array[1][0] = array[0][1];

			array[1][0] = tempValues[0];
			array[2][0] = tempValues[1];
			array[0][1] = tempValues[2];
		}

		Vector3 operator*(const Vector3 &v) const;

		inline Matrix3 operator*(const Matrix3 &a) const {
			Matrix3 out;
			for (unsigned int c = 0; c < 3; ++c) {
				for (unsigned int r = 0; r < 3; ++r) {
					out.array[c][r] = 0.0f;
					for (unsigned int i = 0; i < 3; ++i) {
						out.array[c][r] += this->array[i][r] * a.array[c][i];
					}
				}
			}
			return out;
		}

		//Creates a rotation matrix that rotates by 'degrees' around the 'axis'
		//Analogous to glRotatef
		static Matrix3 Rotation(float degrees, const Vector3 &axis);

		//Creates a scaling matrix (puts the 'scale' vector down the diagonal)
		//Analogous to glScalef
		static Matrix3 Scale(const Vector3 &scale);

		static Matrix3 FromEuler(const Vector3 &euler);
	};

	//Handy string output for the matrix. Can get a bit messy, but better than nothing!
	inline std::ostream& operator<<(std::ostream& o, const Matrix3& m) {
		o << m.array[0][0] << "," << m.array[0][1] << "," << m.array[0][2] << "\n";
		o << m.array[1][0] << "," << m.array[1][1] << "," << m.array[1][2] << "\n";
		o << m.array[2][0] << "," << m.array[2][1] << "," << m.array[2][2] << "\n";
		return o;
	}

	inline std::istream& operator >> (std::istream& i, Matrix3& m) {
		char ignore;
		i >> std::skipws;
		i >> m.array[0][0] >> ignore >> m.array[0][1] >> ignore >> m.array[0][2];
		i >> m.array[1][0] >> ignore >> m.array[1][1] >> ignore >> m.array[1][2];
		i >> m.array[2][0] >> ignore >> m.array[2][1] >> ignore >> m.array[2][2];

		return i;
	}
}