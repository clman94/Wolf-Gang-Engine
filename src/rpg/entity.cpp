#include <rpg/entity.hpp>
#include <rpg/rpg_config.hpp>

#include <engine/utility.hpp>

using namespace rpg;

entity::entity()
{
	mDynamic_depth = true;
	mZ = 0;
	mParallax = 0;
}

void entity::set_dynamic_depth(bool pIs_dynamic)
{
	mDynamic_depth = pIs_dynamic;
}

void entity::set_z(float pZ)
{
	mZ = pZ;
}

float entity::get_z() const
{
	return mZ;
}

void entity::set_parallax(float pParallax)
{
	mParallax = pParallax;
}

void entity::update_depth()
{
	if (mDynamic_depth)
	{
		float ndepth = 
			util::clamp((defs::TILE_DEPTH_RANGE_MAX*0.5f) 
				- (get_position().y / 1000.f)
				, defs::TILE_DEPTH_RANGE_MIN
				, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
}

engine::fvector entity::calculate_draw_position() const
{
	auto renderer = get_renderer();
	if (!renderer)
		return{};
	const engine::fvector z_offset(0, get_z());
	const engine::fvector abs_position = get_absolute_position() - z_offset;
	const engine::fvector parallax_offset = (abs_position - (renderer->get_target_size() / get_unit() / 2)) * mParallax;
	return abs_position + parallax_offset;
}
