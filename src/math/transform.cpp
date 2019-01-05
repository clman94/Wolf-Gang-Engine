#include <wge/math/transform.hpp>

namespace wge::math
{

math::mat33 wge::math::transform::get_matrix() const noexcept
{
	return math::mat33(1)
		.translate(position)
		.rotate(rotation)
		.scale(scale)
		.shear(shear);
}

math::mat33 transform::get_inverse_matrix() const noexcept
{
	return math::mat33(1)
		.shear(-shear)
		.scale(1 / scale)
		.rotate(-rotation)
		.translate(-position);
}

bool transform::is_identity() const noexcept
{
	return position == math::vec2(0, 0)
		&& terminal_angle(rotation) == 0_rad
		&& scale == math::vec2(1, 1)
		&& shear == math::vec2(0, 0);
}

math::vec2 transform::apply_to(const math::vec2 & pVec, const transform_mask & pMask) const noexcept
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

transform transform::apply_to(const transform & pTransform) const noexcept
{
	transform result;
	result.rotation = pTransform.rotation + rotation;
	result.position = apply_to(pTransform.position);
	result.scale = pTransform.scale * scale;
	result.shear = pTransform.shear + shear;
	return result;
}

math::vec2 transform::apply_inverse_to(const math::vec2 & pVec, const transform_mask & pMask) const noexcept
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

transform transform::operator * (const transform& pTransform) const noexcept
{
	return apply_to(pTransform);
}

math::vec2 transform::operator * (const math::vec2& pVec) const noexcept
{
	return apply_to(pVec);
}

transform& transform::operator *= (const transform& pTransform) noexcept
{
	return *this = apply_to(pTransform);
}

transform::operator math::mat33() const noexcept
{
	return get_matrix();
}

std::string transform::to_string() const
{
	return "[Position:" + position.to_string() + "]\n"
		+ "[Rotation:" + std::to_string(rotation.value()) + "]\n"
		+ "[Scale:" + scale.to_string() + "]\n"
		+ "[Shear:" + shear.to_string() + "]";
}



// Generate a matrix that is the inverse of a transform.
// This returns a mat33 because the transform class is unable to
// represent the inverted order of the operations.
// It is recommended that you use the apply_inverse_to method instead
// in most cases.
math::mat33 inverse(const transform & pTransform) noexcept
{
	math::mat33 result(1);
	result
		.scale(1 / pTransform.scale)
		.rotate(-pTransform.rotation)
		.translate(-pTransform.position);
	return result;
}

} // namespace wge::math
