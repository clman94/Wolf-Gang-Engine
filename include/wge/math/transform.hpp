#pragma once

#include <wge/math/math.hpp>
#include <wge/math/matrix.hpp>
#include <wge/math/vector.hpp>

namespace wge::math
{

enum class transform_mask : unsigned int
{
	none     = 0,
	position = 1,
	rotation = 1 << 1,
	scale    = 1 << 2,
	shear    = 1 << 3,
};

inline constexpr transform_mask operator | (const transform_mask& pL, const transform_mask& pR) noexcept
{
	return static_cast<transform_mask>(static_cast<unsigned int>(pL) | static_cast<unsigned int>(pR));
}

inline constexpr bool operator & (const transform_mask& pL, const transform_mask& pR) noexcept
{
	return (static_cast<unsigned int>(pL) & static_cast<unsigned int>(pR)) != 0;
}

inline constexpr bool operator ^ (const transform_mask& pL, const transform_mask& pR) noexcept
{
	return (static_cast<unsigned int>(pL) ^ static_cast<unsigned int>(pR)) != 0;
}

// Represents 4 basic 2d affine transformations.
// The values are applied in this order:
// Shear, scale, rotation, and then the position.
// This is how most objects are represented in this game engine.
class transform
{
public:
	math::vec2 position;
	math::radians rotation;
	math::vec2 scale{ 1, 1 };
	math::vec2 shear{ 0, 0 };

	math::mat33 get_matrix() const noexcept
	{
		return math::mat33(1)
			.translate(position)
			.rotate(rotation)
			.scale(scale)
			.shear(shear);
	}

	math::mat33 get_inverse_matrix() const noexcept
	{
		return math::mat33(1)
			.shear(-shear)
			.scale(1 / scale)
			.rotate(-rotation)
			.translate(-position);
	}

	bool is_identity() const noexcept
	{
		return position == math::vec2(0, 0)
			&& terminal_angle(rotation) == 0
			&& scale == math::vec2(1, 1)
			&& shear == math::vec2(0, 0);
	}

	math::vec2 apply_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept
	{
		math::vec2 result{ pVec };
		if (!(pMask & transform_mask::shear))
			result += shear * math::swap_xy(pVec);
		if (!(pMask & transform_mask::scale))
			result *= scale;
		if (!(pMask & transform_mask::rotation))
			result.rotate(rotation);
		if (!(pMask & transform_mask::position))
			result += position;
		return result;
	}

	transform apply_to(const transform& pTransform) const noexcept
	{
		transform result;
		result.rotation = pTransform.rotation + rotation;
		result.position = apply_to(pTransform.position);
		result.scale = pTransform.scale * scale;
		result.shear = pTransform.shear + shear;
		return result;
	}

	math::vec2 apply_inverse_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept
	{
		math::vec2 result{ pVec };
		if (!(pMask & transform_mask::position))
			result -= position;
		if (!(pMask & transform_mask::rotation))
			result.rotate(-rotation);
		if (!(pMask & transform_mask::scale))
			result /= scale;
		if (!(pMask & transform_mask::shear))
			result -= shear * math::swap_xy(result);
		return result;
	}

	transform operator * (const transform& pTransform) const noexcept
	{
		return apply_to(pTransform);
	}

	transform& operator *= (const transform& pTransform) noexcept
	{
		return *this = apply_to(pTransform);
	}

	math::vec2 operator * (const math::vec2& pVec) const noexcept
	{
		return apply_to(pVec);
	}

	std::string to_string() const
	{
		return "[Position:" + position.to_string() + "]\n"
		     + "[Rotation:" + std::to_string(rotation.value()) + "]\n"
		     + "[Scale:" + scale.to_string() + "]\n"
		     + "[Shear:" + shear.to_string() + "]";
	}
};

// Generate a matrix that is the inverse of a transform.
// This returns a mat33 because the transform class is unable to
// represent the inverted order of the operations.
// It is recommended that you use the apply_inverse_to method instead
// in most cases.
inline math::mat33 inverse(const transform& pTransform) noexcept
{
	math::mat33 result(1);
	result
		.scale(1 / pTransform.scale)
		.rotate(-pTransform.rotation)
		.translate(-pTransform.position);
	return result;
}

} // namespace wge::math
