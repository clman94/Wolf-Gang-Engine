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
//   Shear, scale, rotation, and then the position.
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

	// Check if this transform represents an identity transform
	bool is_identity() const noexcept;

	// Apply this transform to a vector. Returns the new vector.
	math::vec2 apply_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept;
	// Apply this transform to another transform. Returns the new transform.
	transform apply_to(const transform& pTransform) const noexcept;

	// Apply the inverse of this transform to a vector. Returns the new vector.
	math::vec2 apply_inverse_to(const math::vec2& pVec, const transform_mask& pMask = transform_mask::none) const noexcept;

	// Same as apply_to(pTransform)
	transform operator * (const transform& pTransform) const noexcept;
	// Same as apply_to(pVec)
	math::vec2 operator * (const math::vec2& pVec) const noexcept;
	// Same as this = apply_to(pTransform)
	transform& operator *= (const transform& pTransform) noexcept;

	operator math::mat33() const noexcept;

	std::string to_string() const;
};

// Generate a matrix that is the inverse of a transform.
// This returns a mat33 because the transform class is unable to
// represent the inverted order of the operations.
// It is recommended that you use the apply_inverse_to method instead
// in most cases.
math::mat33 inverse(const transform& pTransform) noexcept;

} // namespace wge::math
