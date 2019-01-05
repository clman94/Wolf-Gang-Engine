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
template<std::size_t C, std::size_t R>
class mat
{
public:
	constexpr static const bool is_square = C == R;

	float m[C][R];

	// Initialize with 0's
	mat() noexcept
	{
		fill(0);
	}

	// Construct a matrix as an identity.
	template<typename = std::enable_if<is_square>::type>
	mat(const float& pIdentity_scalar) noexcept
	{
		identity(pIdentity_scalar);
	}

	// Copy matrix
	mat(const mat& pCopy) noexcept
	{
		// Just copy it RAW because why not?
		std::memcpy(this, &pCopy, sizeof(*this));
	}

	// Set to the identity
	template<typename = std::enable_if<is_square>::type>
	mat& identity() noexcept;

	// Set as an identity with a scalar.
	template<typename = std::enable_if<is_square>::type>
	mat& identity(float pScalar) noexcept;

	// Translate x and y with a vec2
	template<typename = std::enable_if<C >= 3 && R >= 2>::type>
	mat& translate(const vec2& pVec2) noexcept;
	// Translate x and y
	template<typename = std::enable_if<C >= 3 && R >= 2>::type>
	mat& translate(float p_x, float p_y) noexcept;
	// Translate x, y, and z
	template<typename = std::enable_if<C >= 4 && R >= 3>::type>
	mat& translate(float p_x, float p_y, float p_z) noexcept;

	// Rotate along the Z-axis
	template<typename = std::enable_if<C >= 2 && R >= 2>::type>
	mat& rotate(const radians& pRadians) noexcept;

	// Scale matrix
	mat& scale(const float& pScalar) noexcept;
	// Scale matrix by a 2d vector
	template<typename = std::enable_if<R >= 2>::type>
	mat& scale(const vec2& pVec2) noexcept;

	template<typename = std::enable_if<C >= 2 && R >= 2>::type>
	mat& shear(const vec2& pVec) noexcept;

	template<typename = std::enable_if<is_square>::type>
	mat& transpose() noexcept;

	template<typename = std::enable_if<is_square>::type>
	float determinant() const noexcept;

	template<typename = std::enable_if<is_square>::type>
	mat& invert() noexcept;

	// Check if the (C-1, R-1) component is 1
	template<typename = std::enable_if<is_square>::type>
	bool is_affine() const noexcept;

	// Set all components to a value
	mat& fill(float pVal) noexcept;

	// Multiply a matrix by another matrix
	template<std::size_t C1, std::size_t R1,
		typename = can_multiply<C, R, C1, R1>::type>
	mat<C1, R> multiply(const mat<C1, R1>& pMat) const noexcept;
	// Multiply a vec2 by this matrix resulting in a new vec2.
	template<typename =
		std::enable_if<is_square && C >= 2>>
	vec2 multiply(const vec2& pVec2) const noexcept;

	template<std::size_t C1, std::size_t R1,
		typename = can_multiply<C, R, C1, R1>::type>
	mat<C1, R> operator * (const mat<C1, R1>& pMat) noexcept
	{
		return multiply(pMat);
	}

	template<typename =
		std::enable_if<is_square && C >= 2>::type>
	vec2 operator * (const vec2& pVec2) const noexcept
	{
		return multiply(pVec2);
	}

	mat operator * (float pScalar) noexcept
	{
		return mat(*this).scale(pScalar);
	}

	std::string to_string() const;
};

typedef mat<3, 3> mat33;
typedef mat<4, 4> mat44;

template<std::size_t C, std::size_t R>
mat<C, R> inverse(const mat<C, R>& pMat) noexcept
{
	return mat<C, R>(pMat).invert();
}

// Set to the identity
template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::identity() noexcept
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = static_cast<float>(c == r ? 1 : 0);
	return *this;
}

// Set as an identity with a scalar.
template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::identity(float pScalar) noexcept
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = c == r ? pScalar : 0;
	return *this;
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::translate(const vec2 & pVec2) noexcept
{
	return translate(pVec2.x, pVec2.y);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::translate(float p_x, float p_y) noexcept
{
	mat<C, R> temp{ 1 };
	temp.m[C - 1][0] += p_x;
	temp.m[C - 1][1] += p_y;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::translate(float p_x, float p_y, float p_z) noexcept
{
	mat<C, R> temp{ 1 };
	temp.m[C - 1][0] += p_x;
	temp.m[C - 1][1] += p_y;
	temp.m[C - 1][2] += p_z;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::rotate(const radians& pRadians) noexcept
{
	float s = std::sinf(pRadians);
	float c = std::cosf(pRadians);
	mat<C, R> temp{ 1 };
	temp.m[0][0] = c; temp.m[1][0] = -s;
	temp.m[0][1] = s; temp.m[1][1] = c;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::scale(const vec2 & pVec2) noexcept
{
	mat<C, R> temp{ 1 };
	temp.m[0][0] = pVec2.x;
	temp.m[1][1] = pVec2.y;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::shear(const vec2 & pVec) noexcept
{
	mat<C, R> temp{ 1 };
	m[1][0] = pVec.x;
	m[0][1] = pVec.y;
	return *this = multiply(temp);
}

template<std::size_t C, std::size_t R>
template<typename>
inline bool mat<C, R>::is_affine() const noexcept
{
	return m[C - 1][R - 1] == 1;
}

template<std::size_t C, std::size_t R>
template<std::size_t C1, std::size_t R1, typename>
inline mat<C1, R> mat<C, R>::multiply(const mat<C1, R1>& pMat) const noexcept
{
	mat<C1, R> result;
	for (std::size_t c = 0; c < C1; c++)
		for (std::size_t r = 0; r < R; r++)
			// Calculate the dot product of the
			// r-th row of the left matrix and the c-th column
			// of the right matrix.
			for (std::size_t i = 0; i < C; i++)
				result.m[c][r] += m[i][r] * pMat.m[c][i];
	return result;
}

template<std::size_t C, std::size_t R>
template<typename>
inline vec2 mat<C, R>::multiply(const vec2 & pVec2) const noexcept
{
	math::mat<1, C> right;
	right.m[0][0] = pVec2.x;
	right.m[0][1] = pVec2.y;
	if (C > 2) // Last component needs to be 1
		right.m[0][C - 1] = 1;
	auto result = multiply(right);
	return{ result.m[0][0], result.m[0][1] };
}

template<std::size_t C, std::size_t R>
inline mat<C, R> & mat<C, R>::scale(const float & pScalar) noexcept
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] *= pScalar;
	return *this;
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::transpose() noexcept
{
	for (std::size_t r = 1; r < R; r++)
		for (std::size_t c = 0; c < r; c++)
			std::swap(m[c][r], m[r][c]);
	return *this;
}

template<std::size_t C, std::size_t R>
template<typename>
inline float mat<C, R>::determinant() const noexcept
{
	return 0.0f;
}

template<>
template<>
inline float mat<2, 2>::determinant<void>() const noexcept
{
	return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}

template<>
template<>
inline float mat<3, 3>::determinant<void>() const noexcept
{
	return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
	     - m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1])
	     + m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
}

template<std::size_t C, std::size_t R>
template<typename>
inline mat<C, R>& mat<C, R>::invert() noexcept
{
	return *this;
}

template<>
template<>
inline mat<3, 3>& mat<3, 3>::invert<void>() noexcept
{
	float idet = 1.f / determinant();
	transpose();
	mat<3, 3> result;
	result.m[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * idet;
	result.m[0][1] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * idet;
	result.m[0][2] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * idet;
	result.m[1][0] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * idet;
	result.m[1][1] =  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * idet;
	result.m[1][2] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * idet;
	result.m[2][0] =  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * idet;
	result.m[2][1] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * idet;
	result.m[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * idet;
	*this = std::move(result);
	return *this;
}

template<std::size_t C, std::size_t R>
inline mat<C, R>& mat<C, R>::fill(float pVal) noexcept
{
	for (std::size_t c = 0; c < C; c++)
		for (std::size_t r = 0; r < R; r++)
			m[c][r] = pVal;
	return *this;
}

template<std::size_t C, std::size_t R>
inline std::string mat<C, R>::to_string() const
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
