#include <rpg/entity.hpp>
#include <rpg/rpg_config.hpp>

#include <engine/utility.hpp>

using namespace rpg;

void entity::set_dynamic_depth(bool pIs_dynamic)
{
	dynamic_depth = pIs_dynamic;
}

void entity::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string& entity::get_name()
{
	return mName;
}

void entity::update_depth()
{
	if (dynamic_depth)
	{
		float ndepth = defs::TILE_DEPTH_RANGE_MAX
			- util::clamp(get_position().y / 32
				, defs::TILE_DEPTH_RANGE_MIN
				, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
}