#ifndef ENGINE_RECT_HPP
#define ENGINE_RECT_HPP

#include "vector.hpp"

namespace engine {

template<typename T>
struct rect
{
	T x, y, w, h;


	rect(T pX = 0, T pY = 0, T pW = 0, T pH = 0)
		: x(pX), y(pY), w(pW), h(pH) {}

	rect(const rect& a)
	{
		set_rect(a);
	}

	rect(const vector<T>& a, const vector<T>& b)
	{
		set_offset(a);
		set_size(b);
	}

	rect& operator=(const rect& r)
	{
		set_rect(r);
		return *this;
	}

	rect& set_rect(const rect& a)
	{
		x = a.x;
		y = a.y;
		w = a.w;
		h = a.h;
		return *this;
	}

	vector<T> get_offset() const
	{
		return vector<T>(x, y);
	}

	rect& set_offset(vector<T> v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}

	vector<T> get_size() const
	{
		return vector<T>(w, h);
	}

	rect& set_size(vector<T> v)
	{
		w = v.x;
		h = v.y;
		return *this;
	}

	rect operator*(T a) const
	{
		return{ x*a, y*a, w*a, h*a };
	}

	vector<T> get_corner() const
	{
		return get_offset() + get_size();
	}

	vector<T> get_center() const
	{
		return get_offset() + (get_size()*0.5f);
	}

	static bool is_intersect(const rect& a, const rect& b)
	{
		if    (a.x < b.x + b.w
			&& a.y < b.y + b.h
			&& a.x + a.w > b.x
			&& a.y + a.h > b.y)
			return true;
		return false;
	}

	static bool is_intersect(const rect& a, const vector<T>& b)
	{
		if    (a.x <= b.x
			&& a.y <= b.y
			&& a.x + a.w > b.x
			&& a.y + a.h > b.y)
			return true;
		return false;
	}

	bool is_intersect(const rect& a) const
	{
		return is_intersect(*this, a);
	}

	bool is_intersect(const vector<T>& a) const
	{
		return is_intersect(*this, a);
	}

#ifdef SFML_RECT_HPP

	rect(const sf::Rect<T>& A)
	{
		x = A.left;
		y = A.top;
		w = A.width;
		h = A.height;
	}

	rect& operator = (const sf::Rect<T>& A)
	{
		x = A.left;
		y = A.top;
		w = A.width;
		h = A.height;
		return *this;
	}

	inline operator sf::Rect<T>() const
	{
		return{ x, y, w, h };
	}

#endif

};

template<typename Tto, typename Torig>
static inline rect<Tto> rect_cast(const rect<Torig>& pRect)
{
	return
	{
		static_cast<Tto>(pRect.x),
		static_cast<Tto>(pRect.y),
		static_cast<Tto>(pRect.w),
		static_cast<Tto>(pRect.h)
	};
}


template<typename T1, typename T2>
static inline rect<T1> scale(rect<T1> a, T2 b)
{
	return{ a.get_offset()*b, a.get_size()*b };
}

template<typename T1, typename T2>
static inline rect<T1> scale(rect<T1> a, vector<T2> b)
{
	return{ a.x*b.x,a.y*b.y, a.w*b.x, a.h*b.y };
}

typedef rect<float> frect;
typedef rect<int> irect;

}

#endif