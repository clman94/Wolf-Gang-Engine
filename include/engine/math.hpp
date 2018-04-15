#pragma once

#include <cmath>

namespace math
{

// Returns the always-positive remainder
inline float pfmod(float a, float b)
{
	return std::fmod(std::fmod(a, b) + b, b);
}

}