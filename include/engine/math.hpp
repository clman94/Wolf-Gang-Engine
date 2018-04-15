#pragma once

#include <cmath>

namespace math
{

// Returns the always-positive remainder
inline float pfmodf(float a, float b)
{
	return std::fmodf(std::fmodf(a, b) + b, b);
}

}