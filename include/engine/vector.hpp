#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cmath>
#include <type_traits>
#include <string>

namespace
{

// Estimate Radians from degree
template<typename T>
T degree_to_radian(T pDegree)
{
	return pDegree * static_cast<T>(0.01745329252);
}

}

namespace engine
{

template<typename T>
struct vector
{
	typedef T type;

	T x, y;

	static vector as_x(T pVal)
	{
		return{ pVal, 0 };
	}
	static vector as_y(T pVal)
	{
		return{ 0, pVal };
	}

	static vector x_only(const vector& pVec)
	{
		return{ pVec.x, 0 };
	}
	static vector y_only(const vector& pVec)
	{
		return{ 0, pVec.y };
	}

	vector(T _x = 0, T _y = 0)
		: x(_x), y(_y)
	{}

	vector(const vector<T>& pVector)
	{
		x = pVector.x;
		y = pVector.y;
	}

	T distance() const
	{
		return std::sqrt(std::pow(x, 2) + std::pow(y, 2));
	}

	T distance(const vector& A) const
	{
		return std::sqrt(std::pow(A.x - x, 2) + std::pow(A.y - y, 2));
	}

	T manhattan() const
	{
		return std::abs(x) + std::abs(y);
	}

	T manhattan(const vector& A) const
	{
		return std::abs(A.x - x) + std::abs(A.y - y);
	}

	vector& rotate(T pDegrees)
	{
		const T r = degree_to_radian(pDegrees);
		T x1 = x*std::cos(r) - y*std::sin(r);
		T y1 = y*std::cos(r) + x*std::sin(r);
		x = x1;
		y = y1;
		return *this;
	}

	vector& rotate(const vector& pOrigin, T pDegrees)
	{
		*this -= pOrigin;
		rotate(pDegrees);
		*this += pOrigin;
		return *this;
	}

	vector operator - () const
	{
		return{ -x, -y };
	}

	vector operator + (const vector& A) const
	{
		return{ x + A.x, y + A.y};
	}

	vector operator - (const vector& A) const
	{
		return{ x - A.x, y - A.y};
	}

	vector operator * (const vector& A) const
	{
		return{ x * A.x, y * A.y};
	}

	vector operator / (const vector& A) const
	{
		return{ x / A.x, y / A.y };
	}
	vector operator % (const vector& A) const
	{
		return{ std::fmod(x, A.x), std::fmod(y, A.y) };
	}

	vector operator * (T A) const
	{
		return{ x * A, y * A};
	}

	vector operator / (T A) const
	{
		return{ x / A, y / A };
	}

	vector& operator *= (T A)
	{
		x *= A;
		y *= A;
		return *this;
	}

	vector& operator /= (T A)
	{
		x /= A;
		y /= A;
		return *this;
	}

	vector& operator = (const vector& A)
	{
		x = A.x;
		y = A.y;
		return *this;
	}


	vector& operator += (const vector& A)
	{
		x += A.x;
		y += A.y;
		return *this;
	}

	vector& operator -= (const vector& A)
	{
		x -= A.x;
		y -= A.y;
		return *this;
	}

	vector& operator *= (const vector& A)
	{
		x *= A.x;
		y *= A.y;
		return *this;
	}

	vector& operator /= (const vector& A)
	{
		x /= A.x;
		y /= A.y;
		return *this;
	}

	vector& operator %= (const vector& A)
	{
		x = std::fmod(x, A.x);
		y = std::fmod(y, A.y);
		return *this;
	}

	template<typename T1>
	bool operator==(const vector<T1>& R) const
	{
		return (x == R.x) && (y == R.y);
	}

	vector& floor()
	{
		if (!std::is_floating_point<T>::value)
			return *this;
		x = std::floor(x);
		y = std::floor(y);
		return *this;
	}

	vector& round()
	{
		if (!std::is_floating_point<T>::value)
			return *this;
		x = std::round(x);
		y = std::round(y);
		return *this;
	}

	vector& flip()
	{
		std::swap(x, y);
		return *this;
	}

	vector& normalize()
	{
		T d = distance();
		if (d == 0)
			return *this;
		x /= d;
		y /= d;
		return *this;
	}

	T angle() const
	{
		if (!std::is_floating_point<T>::value)
			return 0;

		T a = std::atan2(y, x) * static_cast<T>(180 / 3.14159265); // Compiler complains without these casts
		return std::fmod(a + 360, static_cast<T>(360));
	}

	T angle(const vector& A) const
	{
		if (!std::is_floating_point<T>::value)
			return 0;

		T a = std::atan2(y - A.y, x - A.x) * static_cast<T>(180 / 3.14159265); // Compiler complains without these casts
		return std::fmod(a + 360, static_cast<T>(360));
	}

	vector& abs()
	{
		x = std::abs(x);
		y = std::abs(y);
		return *this;
	}

	std::string to_string() const
	{
		return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
	}

	bool has_zero() const
	{
		return x == 0 || y == 0;
	}

#ifdef SFML_VECTOR2_HPP
	vector(const sf::Vector2<T>& A)
	{
		x = A.x;
		y = A.y;
	}

	/*vector& operator = (const sf::Vector2<T>& A)
	{
		x = A.x;
		y = A.y;
		return *this;
	}*/

	operator sf::Vector2<T>() const
	{
		return{ x, y };
	}

#endif

	template<typename Tto>
	explicit operator engine::vector<Tto>() const
	{
		return{ static_cast<Tto>(px), static_cast<Tto>(y) };
	}

	template<typename>
	friend struct vector;
};

template<typename T1, typename T2>
bool operator<(const vector<T1>& L, const vector<T2>& R)
{
	return (L.y < R.y) || ((L.y == R.y) && (L.x < R.x));
}

template<typename T1, typename T2>
bool operator>(const vector<T1>& L, const vector<T2>& R)
{
	return (L.y > R.y) || ((L.y == R.y) && (L.x > R.x));
}

template<typename T1, typename T2>
bool operator!=(const vector<T1>& L, const vector<T2>& R)
{
	return (L.x != R.x) || (L.y != R.y);
}

template<typename Tto, typename Tfrom>
vector<Tto> vector_cast(const vector<Tfrom>& pOrig)
{
	return{ static_cast<Tto>(pOrig.x), static_cast<Tto>(pOrig.y) };
}

typedef vector<int>    ivector;
typedef vector<unsigned int> uvector;
typedef vector<float>  fvector;
typedef vector<double> dvector;


}

#endif
