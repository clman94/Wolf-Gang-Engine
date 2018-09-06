#include <wge/math/matrix.hpp>

namespace wge
{
namespace math
{

inline mat44 ortho(float left, float right, float top, float bottom)
{
	mat44 result(1);
	result.m[0][0] = 2.f / (right - left);
	result.m[1][1] = 2.f / (top - bottom);
	result.m[3][0] = -(right + left) / (right - left);
	result.m[3][1] = -(top + bottom) / (top - bottom);
	return result;
}

} // namespace math
} // namespace wge