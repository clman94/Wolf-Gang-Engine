#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cmath>
#include <type_traits>

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
	T x, y;

	vector(T _x = 0, T _y = 0)
		: x(_x), y(_y)
	{}

	template<typename T1>
	vector(const vector<T1>& v)
	{
		x = (T)v.x;
		y = (T)v.y;
	}

	T distance() const
	{
		return std::sqrt(std::pow(x, 2) + std::pow(y, 2));
	}

	template<typename T1>
	T distance(const vector<T1>& A) const
	{
		return std::sqrt(std::pow(A.x - x, 2) + std::pow(A.y - y, 2));
	}

	T manhattan() const
	{
		return std::abs(x) + std::abs(y);
	}

	template<typename T1>
	T manhattan(const vector<T1>& A) const
	{
		return std::abs(A.x - x) + std::abs(A.y - y);
	}

	vector& rotate(T pDegrees)
	{
		const T r = degree_to_radian(pDegrees);
		float x1 = x*std::cos(r) - y*std::sin(r);
		float y1 = y*std::cos(r) + x*std::sin(r);
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

	template<typename T1>
	vector operator + (const vector<T1>& A) const
	{
		return{ x + (T)A.x, y + (T)A.y};
	}

	template<typename T1>
	vector operator - (const vector<T1>& A) const
	{
		return{ x - (T)A.x, y - (T)A.y};
	}

	template<typename T1>
	vector operator * (const vector<T1>& A) const
	{
		return{ x * (T)A.x, y * (T)A.y};
	}

	// No division, high chance of dividing by zero.

	template<typename T1>
	vector operator * (T1 A) const
	{
		return{ x * (T)A, y * (T)A};
	}

	template<typename T1>
	vector operator / (T1 A) const
	{
		return{ x / (T)A, y / (T)A };
	}

	vector operator - () const
	{
		return{ -x, -y};
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

	template<typename T1>
	vector& operator = (const vector<T1>& A)
	{
		x = (T)A.x;
		y = (T)A.y;
		return *this;
	}


	template<typename T1>
	vector& operator += (const vector<T1>& A)
	{
		x += (T)A.x;
		y += (T)A.y;
		return *this;
	}

	template<typename T1>
	vector& operator -= (const vector<T1>& A)
	{
		x -= (T)A.x;
		y -= (T)A.y;
		return *this;
	}

	template<typename T1>
	vector& operator *= (const vector<T1>& A)
	{
		x *= (T)A.x;
		y *= (T)A.y;
		return *this;
	}

	template<typename T1>
	vector& operator /= (const vector<T1>& A)
	{
		x /= (T)A.x;
		y /= (T)A.y;
		return *this;
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

#ifdef SFML_VERTEX_HPP
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

	inline operator sf::Vector2<T>() const
	{
		return{ x, y };
	}

#endif

	// No division, high chance of dividing by zero.

	template<typename>
	friend struct vector;
};


template<typename T1, typename T2>
bool operator<(const vector<T1>& L, const vector<T2>& R)
{
	return (L.y < R.y) || ((L.y == R.y) && (L.x < R.x));
}

template<typename T1, typename T2>
bool operator!=(const vector<T1>& L, const vector<T2>& R)
{
	return (L.x != R.x) || (L.y != R.y);
}

template<typename T1, typename T2>
bool operator==(const vector<T1>& L, const vector<T2>& R)
{
	return (L.x == R.x) && (L.y == R.y);
}


typedef vector<int>    ivector;
typedef vector<float>  fvector;
typedef vector<double> dvector;

}

#endif
