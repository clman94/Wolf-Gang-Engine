#pragma once

#include <cmath>

namespace math
{

static const float PI = 3.14159265f;

// Returns always-positive unless b < 0
inline int pmod(int a, int b)
{
	return (a % b + b) % b;
}

// Returns always-positive unless b < 0
inline float pmod(float a, float b)
{
	return std::fmod(std::fmod(a, b) + b, b);
}

inline unsigned int mod(unsigned int a, unsigned int b)
{
	return a % b; 
}

inline int mod(int a, int b)
{
	return a % b;
}

inline float mod(float a, float b)
{
	return std::fmod(a, b);
}

inline double mod(double a, double b)
{
	return std::fmod(a, b);
}

}