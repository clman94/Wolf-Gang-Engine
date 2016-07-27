#ifndef ENGINE_RECT_HPP
#define ENGINE_RECT_HPP

#include "vector.hpp"

namespace engine {

template<typename T>
struct rect
{
	T x, y, w, h;
	rect(T _x = 0, T _y = 0, T _w = 0, T _h = 0)
		: x(_x), y(_y), w(_w), h(_h) {}
	rect(const rect& a)
	{
		x = a.x;
		y = a.y;
		w = a.w;
		h = a.h;
	}

	rect(const vector<T>& a, const vector<T>& b)
	{
		x = a.x;
		y = a.y;
		w = b.x;
		h = b.y;
	}

	rect& operator=(const rect& r)
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
		return *this;
	}

	void set_rect(const rect& a)
	{
		x = a.x;
		y = a.y;
		w = a.w;
		h = a.h;
	}

	vector<T> get_offset()
	{
		return vector<T>(x, y);
	}

	vector<T> get_size()
	{
		return vector<T>(w, h);
	}

	void set_offset(vector<T> v)
	{
		x = v.x;
		y = v.y;
	}

	void set_size(vector<T> v)
	{
		w = v.x;
		h = v.y;
	}

	static bool is_intersect(const rect& a, const rect& b)
	{
		if    (a.x <= b.x + b.w
			&& a.y <= b.y + b.h
			&& a.x + a.w >= b.x
			&& a.y + a.h >= b.y)
			return true;
		return false;
	}

	static bool is_intersect(const rect& a, const vector<T>& b)
	{
		if    (a.x <= b.x
			&& a.y <= b.y
			&& a.x + a.w >= b.x
			&& a.y + a.h >= b.y)
			return true;
		return false;
	}

	bool is_intersect(const rect& a)
	{
		return is_intersect(*this, a);
	}

	bool is_intersect(const vector<T>& a)
	{
		return is_intersect(*this, a);
	}
};

typedef rect<float> frect;
typedef rect<int> irect;

}

#endif