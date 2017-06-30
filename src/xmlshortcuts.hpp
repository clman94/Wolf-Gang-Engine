#ifndef XMLSHORTCUTS_HPP
#define XMLSHORTCUTS_HPP

#include <engine/vector.hpp>
#include <engine/rect.hpp>

#include "../tinyxml2/tinyxml2.h"
#include <engine/parsers.hpp>
#include <engine/log.hpp>

#include <cassert>

namespace util{
namespace shortcuts{

static engine::fvector load_vector_float_att(tinyxml2::XMLElement *e)
{
	assert(e != nullptr);
	return{ e->FloatAttribute("x"), e->FloatAttribute("y") };
}

static engine::frect load_rect_float_att(tinyxml2::XMLElement *e)
{
	assert(e != nullptr);
	return{ 
		e->FloatAttribute("x"), 
		e->FloatAttribute("y"),
		e->FloatAttribute("w"),
		e->FloatAttribute("h")
	};
}

static void save_rect_float_att(tinyxml2::XMLElement *e, const engine::frect& pRect)
{
	assert(e != nullptr);
	e->SetAttribute("x", pRect.x);
	e->SetAttribute("y", pRect.y);
	e->SetAttribute("w", pRect.w);
	e->SetAttribute("h", pRect.h);
}

static bool validate_potential_xml_name(const std::string& pText)
{
	for (auto i : pText)
	{
		if (parsers::is_whitespace(i))
		{
			logger::error("Cannot have whitespace in name");
			return false;
		}
		// TODO: Check docs for what is not allowed in xml tag names
	}
	return true;
}

}
}

#endif // !XMLSHORTCUTS_HPP
