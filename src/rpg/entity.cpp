#include <rpg/entity.hpp>
#include <rpg/rpg_config.hpp>

#include <engine/utility.hpp>

using namespace rpg;

entity::entity()
{
	mDynamic_depth = true;
	mZ = 0;
}

void entity::set_dynamic_depth(bool pIs_dynamic)
{
	mDynamic_depth = pIs_dynamic;
}

void entity::set_z(float pZ)
{
	mZ = pZ;
	update_z();
}

float entity::get_z() const
{
	return mZ;
}

void entity::update_depth()
{
	if (mDynamic_depth)
	{
		float ndepth = defs::TILE_DEPTH_RANGE_MAX
			- util::clamp(get_position().y / get_unit()
				, defs::TILE_DEPTH_RANGE_MIN
				, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
}