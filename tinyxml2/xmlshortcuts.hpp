#ifndef XMLSHORTCUTS_HPP
#define XMLSHORTCUTS_HPP

#include "../vector.hpp"
#include "../rect.hpp"

#include "tinyxml2.h"

#include <cassert>

namespace util{
namespace shortcuts{

static engine::fvector vector_float_att(tinyxml2::XMLElement *e)
{
	assert(e != nullptr);
	return{ e->FloatAttribute("x"), e->FloatAttribute("y") };
}

static engine::frect rect_float_att(tinyxml2::XMLElement *e)
{
	assert(e != nullptr);
	return{ 
		e->FloatAttribute("x"), 
		e->FloatAttribute("y"),
		e->FloatAttribute("w"),
		e->FloatAttribute("h")
	};
}

}
}

#endif // !XMLSHORTCUTS_HPP
