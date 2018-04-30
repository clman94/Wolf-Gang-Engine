#ifndef ENGINE_RECT_HPP
#define ENGINE_RECT_HPP

#include "vector.hpp"

namespace engine {

template<typename T>
struct rect
{
	union{
		struct{
			T x, y, w, h;
		};
		T components[4];
	};

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

	bool operator==(const rect& r) const
	{
		return x == r.x &&
			y == r.y &&
			w == r.w &&
			h == r.h;
	}

	bool operator!=(const rect& r) const
	{
		return !operator==(r);
	}

	rect& operator=(const rect& r)
	{
		set_rect(r);
		return *this;
	}

	rect& operator+=(const rect& r)
	{
		x += r.x;
		y += r.y;
		w += r.w;
		h += r.h;
		return *this;
	}

	rect& operator*=(const T& r)
	{
		x *= r;
		y *= r;
		w *= r;
		h *= r;
		return *this;
	}

	rect operator+(const rect& r) const
	{
		return{
			x + r.x,
			y + r.y,
			w + r.w,
			h + r.h,
		};
	}

	rect operator*(const T& a) const
	{
		return{ x*a, y*a, w*a, h*a };
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

	vector<T> get_vertex(std::size_t pIndex) const
	{
		switch (pIndex % 4)
		{
		case 0: return { x, y };
		case 1: return { x + w, y };
		case 2: return { x + w, y + h };
		case 3: return { x, y + h };
		}
		return {};
	}

	vector<T> get_corner() const
	{
		return get_offset() + get_size();
	}

	vector<T> get_center() const
	{
		return get_offset() + (get_size()*0.5f);
	}

	bool is_intersect(const rect& a) const
	{
		return x < a.x + a.w
			&& y < a.y + a.h
			&& x + w > a.x
			&& y + h > a.y;
	}

	bool is_intersect(const vector<T>& a) const
	{
		return x <= a.x
			&& y <= a.y
			&& x + w > a.x
			&& y + h > a.y;
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

	operator sf::Rect<T>() const
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

typedef rect<float> frect;
typedef rect<int> irect;

}

#endif