
class color
{
public:
	union {
		struct {
			float r, g, b, a;
		};
		float components[4];
	};

	color() :
		r(0), g(0), b(0), a(1)
	{}
	color(const color& pColor) :
		r(pColor.r), g(pColor.g), b(pColor.b), a(pColor.a)
	{}
	color(float pR, float pG, float pB) :
		r(pR), g(pG), b(pB), a(1)
	{}
	color(float pR, float pG, float pB, float pA) :
		r(pR), g(pG), b(pB), a(pA)
	{}

	color operator+(const color& pColor) const;
	color operator-(const color& pColor) const;
	color operator*(const color& pColor) const;
	color operator/(const color& pColor) const;

	color& operator=(const color& pColor) = default;
	color& operator+=(const color& pColor);
	color& operator-=(const color& pColor);
	color& operator*=(const color& pColor);
	color& operator/=(const color& pColor);
};