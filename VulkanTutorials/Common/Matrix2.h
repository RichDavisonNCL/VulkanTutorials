/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Vector2.h"
#include <assert.h>
namespace NCL::Maths {
	class Matrix2 {
	public: 
		float	array[2][2];
	public:
		Matrix2(void);
		Matrix2(float elements[4]);

		~Matrix2(void) = default;

		void ToZero();

		Matrix2& SetRow(unsigned int row, const Vector2 &val) {
			assert(row < 2);
			array[0][row] = val.x;
			array[1][row] = val.y;
			return *this;
		}

		Matrix2& SetColumn(unsigned int column, const Vector2 &val) {
			assert(column < 2);
			array[column][0] = val.x;
			array[column][1] = val.y;
			return *this;
		}

		Vector2 GetRow(unsigned int row) const {
			assert(row < 2);
			return Vector2(array[0][row],array[1][row]);
		}

		Vector2 GetColumn(unsigned int column) const {
			assert(column < 2);
			return Vector2(array[column][0], array[column][1]);
		}

		Vector2 GetDiagonal() const {
			return Vector2(array[0][0], array[1][1]);
		}

		void	SetDiagonal(const Vector2 &in) {
			array[0][0] = in.x;
			array[1][1] = in.y;
		}

		inline Vector2 operator*(const Vector2 &v) const {
			Vector2 vec;

			vec.x = v.x*array[0][0] + v.y*array[1][0];
			vec.y = v.x*array[0][1] + v.y*array[1][1];

			return vec;
		};

		static Matrix2 Rotation(float degrees);

		//Handy string output for the matrix. Can get a bit messy, but better than nothing!
		inline friend std::ostream& operator<<(std::ostream& o, const Matrix2& m);
	};

	//Handy string output for the matrix. Can get a bit messy, but better than nothing!
	inline std::ostream& operator<<(std::ostream& o, const Matrix2& m) {
		o << "Mat2(";
		o << "\t" << m.array[0][0] << "," << m.array[0][1] << "\n";
		o << "\t\t" << m.array[1][0] << "," << m.array[1][1] << "\n";
		return o;
	}
}