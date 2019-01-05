#pragma once

#include <wge/math/math.hpp>
#include <wge/math/matrix.hpp>
#include <wge/math/vector.hpp>
#include <wge/util/enum.hpp>

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

ENUM_CLASS_FLAG_OPERATORS(transform_mask);

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

	math::mat33 get_matrix() const noexcept;

	math::mat33 get_inverse_matrix() const noexcept;

	bool is_identity() const noexcept;

	math::vec2 apply_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept;

	transform apply_to(const transform& pTransform) const noexcept;

	math::vec2 apply_inverse_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept;

	transform operator * (const transform& pTransform) const noexcept;
	math::vec2 operator * (const math::vec2& pVec) const noexcept;

	transform& operator *= (const transform& pTransform) noexcept;


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
math::mat33 inverse(const transform& pTransform) noexcept;

} // namespace wge::math
