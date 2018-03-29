#pragma once

#include <cstdint>

namespace engine {

// TODO: Change this to floating point
typedef float color_t;

class color
{
public:

	union {
		struct {
			color_t r, g, b, a;
		};
		color_t components[4];
	};


	color(
		color_t pR = 0,
		color_t pG = 0,
		color_t pB = 0,
		color_t pA = 1.f);

	color(const color& pColor);

#ifdef SFML_COLOR_HPP
	color(const sf::Color& pColor);
	operator sf::Color() const;

#endif // SFML_COLOR_HPP

	float intensity() const;

	color& operator+=(const color& pColor);
	color& operator-=(const color& pColor);
	color& operator*=(const color& pColor);
	color& operator/=(const color& pColor);

	color operator+(const color& pColor) const;
	color operator-(const color& pColor) const;
	color operator*(const color& pColor) const;
	color operator/(const color& pColor) const;

	bool get_mask(size_t pIndex) const;
	void set_mask(size_t pIndex, bool pEnabled);
	void set_mask(bool pR, bool pG, bool pB, bool pA);

	float get_hue() const;

	color& clamp(float min, float max);
	color& clamp();

private:
	bool mask[4];

	void default_mask();
};

namespace color_preset
{
static const color opaque(1, 1, 1, 1);
static const color white(1, 1, 1, 1);
static const color black(0, 0, 0, 1);
}

}