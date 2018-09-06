#pragma once

#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>

namespace wge
{
namespace math
{

// Template for checking if 2 matrices can be
// multiplied.
template<std::size_t C, std::size_t R,
	std::size_t C1, std::size_t R1>
	struct can_multiply :
	// Columns of left matrix = Rows of right matrix
	std::enable_if<C == R1>
{};

// Templated matrix class
template<std::size_t C, std::size_t R, class T>
class mat
{
public:
	constexpr static const bool is_square = C == R;

	T m[C][R];

	// Initialize with 0's
	mat()
	{
		fill(0);
	}

	// Construct a matrix as an identity with a scalar.
	template<typename = std::enable_if<is_square>::type>
	mat(const T& pIdentity_scalar)
	{
		identity(pIdentity_scalar);
	}

	// Copy matrix
	mat(const mat& pCopy)
	{
		// Just copy it RAW because why not?
		std::memcpy(this, &pCopy, sizeof(*this));
	}

	// Set to the identity
	template<typename = std::enable_if<is_square>::type>
	mat& identity();

	// Set as an identity with a scalar.
	template<typename = std::enable_if<is_square>::type>
	mat& identity(const T& pScalar);

	// Translate x and y with a vec2
	template<typename = std::enable_if<C >= 3 && R >= 2>::type>
	mat& translate(const vec2& pVec2);
	// Translate x and y
	template<typename = std::enable_if<C >= 3 && R >= 2>::type>
	mat& translate(T p_x, T p_y);
	// Translate x, y, and z
	template<typename = std::enable_if<C >= 4 && R >= 3>::type>
	mat& translate(T p_x, T p_y, T p_z);

	// Rotate along the Z-axis
	template<typename = std::enable_if<C >= 2 && R >= 2>::type>
	mat& rotate(const radians& pRadians);

	// Scale matrix
	mat& scale(const T& pScalar);
	// Scale matrix by a 2d vector
	template<typename = std::enable_if<R >= 2>::type>
	mat& scale(const vec2& pVec2);

	// Check if the (C-1, R-1) component is 1
	template<typename = std::enable_if<is_square>::type>
	bool is_affine() const;

	// Set all components to a value
	mat& fill(const T& pVal);

	// Multiply a matrix by another matrix
	template<std::size_t C1, std::size_t R1,
		typename = can_multiply<C, R, C1, R1>::type>
	mat<C1, R, T> multiply(const mat<C1, R1, T>& pMat);
	// Multiply a vec2 by this matrix resulting in a new vec2.
	template<typename =
		std::enable_if<is_square && C >= 2 &&
			std::is_same<float, T>::value>::type>
	vec2 multiply(const vec2& pVec2);

	template<std::size_t C1, std::size_t R1,
		typename = can_multiply<C, R, C1, R1>::type>
	mat<C1, R, T> operator * (const mat<C1, R1, T>& pMat)
	{
		return multiply(pMat);
	}

	template<typename =
		std::enable_if<is_square && C >= 2 &&
			std::is_same<float, T>::value>::type>
	vec2 operator * (const vec2& pVec2)
	{
		return multiply(pVec2);
	}

	mat operator * (const T& pScalar)
	{
		return mat(*this).scale(pScalar);
	}

	std::string to_string() const;
};

typedef mat<3, 3, float> mat33;
typedef mat<4, 4, float> mat44;

// Set to the identity
template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::identity()
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = static_cast<T>(c == r ? 1 : 0);
	return *this;
}

// Set as an identity with a scalar.
template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::identity(const T & pScalar)
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = static_cast<T>(c == r ? pScalar : 0);
	return *this;
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::translate(const vec2 & pVec2)
{
	return translate(pVec2.x, pVec2.y);
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::translate(T p_x, T p_y)
{
	mat<C, R, T> temp{ 1 };
	temp.m[C - 1][0] += p_x;
	temp.m[C - 1][1] += p_y;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::translate(T p_x, T p_y, T p_z)
{
	mat<C, R, T> temp{ 1 };
	temp.m[C - 1][0] += p_x;
	temp.m[C - 1][1] += p_y;
	temp.m[C - 1][2] += p_z;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::rotate(const radians& pRadians)
{
	float s = std::sinf(pRadians);
	float c = std::cosf(pRadians);
	mat<C, R, T> temp{ 1 };
	temp.m[0][0] = c; temp.m[1][0] = -s;
	temp.m[0][1] = s; temp.m[1][1] = c;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline mat<C, R, T>& mat<C, R, T>::scale(const vec2 & pVec2)
{
	mat<C, R, T> temp{ 1 };
	temp.m[0][0] = pVec2.x;
	temp.m[1][1] = pVec2.y;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline bool mat<C, R, T>::is_affine() const
{
	return m[C - 1][R - 1] == 1;
}

template<std::size_t C, std::size_t R, class T>
template<std::size_t C1, std::size_t R1, typename>
inline mat<C1, R, T> mat<C, R, T>::multiply(const mat<C1, R1, T>& pMat)
{
	mat<C1, R, T> result;
	for (std::size_t c = 0; c < C1; c++)
		for (std::size_t r = 0; r < R; r++)
			// Calculate the dot product of the
			// r-th row of the left matrix and the c-th column
			// of the right matrix.
			for (std::size_t i = 0; i < C; i++)
				result.m[c][r] += m[i][r] * pMat.m[c][i];
	return result;
}

template<std::size_t C, std::size_t R, class T>
template<typename>
inline vec2 mat<C, R, T>::multiply(const vec2 & pVec2)
{
	math::mat<1, C, float> right;
	right.m[0][0] = pVec2.x;
	right.m[0][1] = pVec2.y;
	if (C > 2) // Last component needs to be 1
		right.m[0][C - 1] = 1;
	auto result = *this * right;
	return{ result.m[0][0], result.m[0][1] };
}

template<std::size_t C, std::size_t R, class T>
inline mat<C, R, T > & mat<C, R, T>::scale(const T & pScalar)
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] *= pScalar;
	return *this;
}

template<std::size_t C, std::size_t R, class T>
inline mat<C, R, T>& mat<C, R, T>::fill(const T & pVal)
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = pVal;
	return *this;
}

template<std::size_t C, std::size_t R, class T>
inline std::string mat<C, R, T>::to_string() const
{
	std::string result;
	for (std::size_t r = 0; r < R; r++)
	{
		for (std::size_t c = 0; c < C; c++)
		{
			result += std::to_string(m[c][r]);
			result += " ";
		}
		result += "\n";
	}
	return result;
}

} // namespace math
} // namespace wge
