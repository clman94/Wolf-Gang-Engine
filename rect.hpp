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
};

typedef rect<float> frect;
typedef rect<int> irect;

}

#endif