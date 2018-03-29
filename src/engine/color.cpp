#include <SFML/Graphics/Color.hpp>

#include <engine/color.hpp>
#include <engine/utility.hpp>

#include <cmath>

using namespace engine;

color::color(color_t pR, color_t pG, color_t pB, color_t pA)
	: r(pR), g(pG), b(pB), a(pA)
{
	default_mask();
}

color::color(const color & pColor)
{
	for (int i = 0; i < 4; i++)
		components[i] = pColor.components[i];
}

color::color(const sf::Color& pColor)
{
	r = static_cast<float>(pColor.r) / 255.f;
	g = static_cast<float>(pColor.g) / 255.f;
	b = static_cast<float>(pColor.b) / 255.f;
	a = static_cast<float>(pColor.a) / 255.f;
}

color::operator sf::Color() const
{
	return{ 
		static_cast<sf::Uint8>(r * 255),
		static_cast<sf::Uint8>(g * 255), 
		static_cast<sf::Uint8>(b * 255), 
		static_cast<sf::Uint8>(a * 255)
	};
}

float color::intensity() const
{
	return ((r + b + g) / 3.0f)*a;
}

color& color::operator+=(const color& pColor)
{
	for (int i = 0; i < 4; i++)
		if (!mask[i])
			components[i] += pColor.components[i];
	return *this;
}

color& color::operator-=(const color& pColor)
{
	for (int i = 0; i < 4; i++)
		if (!mask[i])
			components[i] -= pColor.components[i];
	return *this;
}


color& color::operator*=(const color& pColor)
{
	for (int i = 0; i < 4; i++)
		if (!mask[i])
			components[i] *= pColor.components[i];
	return *this;
}

color& color::operator/=(const color& pColor)
{
	for (int i = 0; i < 4; i++)
		if (!mask[i])
			components[i] /= pColor.components[i];
	return *this;
}

color color::operator+(const color& pColor) const
{
	color ret(*this);
	ret += pColor;
	return ret;
}

color color::operator-(const color& pColor) const
{
	color ret(*this);
	ret -= pColor;
	return ret;
}

color color::operator*(const color& pColor) const
{
	color ret(*this);
	ret *= pColor;
	return ret;
}

color color::operator/(const color& pColor) const
{
	color ret(*this);
	ret /= pColor;
	return ret;
}

bool color::get_mask(size_t pIndex) const
{
	if (pIndex >= 4)
		return false;
	return mask[pIndex];
}

void color::set_mask(size_t pIndex, bool pEnabled)
{
	if (pIndex >= 4)
		return;
	mask[pIndex] = pEnabled;
}

void color::set_mask(bool pR, bool pG, bool pB, bool pA)
{
	mask[0] = pR;
	mask[1] = pG;
	mask[2] = pB;
	mask[3] = pA;
}

float color::get_hue() const
{
	/*float max = std::fmaxf(r, std::fmaxf(g, b));
	float min = std::fminf(r, std::fminf(g, b));
	if ()*/
	return 0.0f;
}

color& color::clamp(float min, float max)
{
	for (int i = 0; i < 4; i++)
		components[i] = util::clamp<float>(components[i], min, max);
	return *this;
}

color& color::clamp()
{
	return clamp(0, 1);
}

void color::default_mask()
{
	set_mask(false, false, false, true);
}
