#ifndef VECTOR_HPP
#define VECTOR_HPP

namespace engine
{

// Template expects  int, double, float, etc...
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

	template<typename T1>
	vector operator + (const vector<T1>& A)
	{
		return{ x + (T)A.x, y + (T)A.y};
	}

	template<typename T1>
	vector operator - (const vector<T1>& A)
	{
		return{ x - (T)A.x, y - (T)A.y};
	}

	template<typename T1>
	vector operator * (const vector<T1>& A)
	{
		return{ x * (T)A.x, y * (T)A.y};
	}

	// No division, high chance of dividing by zero.

	template<typename T1>
	vector operator * (const T1& A)
	{
		return{ x * (T)A, y * (T)A};
	}

	template<typename T1>
	vector operator / (const T1& A)
	{
		return{ x / (T)A, y / (T)A };
	}

	vector operator - ()
	{
		return{ -x, -y};
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

	// No division, high chance of dividing by zero.

	template<typename>
	friend struct vector;
};

template<typename T1, typename T2>
bool operator<(const vector<T1>& L, const vector<T2>& R)
{
	return (L.x + L.y) < (R.x + R.y);
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

template<typename T>
static vector<T> scale(vector<T> a, T b)
{
	return a*b;
}



typedef vector<float> fvector;
typedef vector<int>   ivector;
}

#endif