#include <wge/core/component.hpp>

using namespace wge;
using namespace wge::core;

const void component::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string& component::get_name() const
{
	return mName;
}
