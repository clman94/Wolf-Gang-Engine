#include <wge/core/object_resource.hpp>
#include <wge/scripting/script_engine.hpp>
#include <wge/util/ipair.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/physics/physics_component.hpp>

namespace wge::core
{

void object_resource::generate_object(core::object& pObj, const asset_manager& pAsset_mgr)
{
	pObj.add_component(math::transform{});

	if (display_sprite.is_valid())
	{
		pObj.add_component(graphics::sprite_component{ pAsset_mgr.get_asset(display_sprite) });
	}

	if (is_collision_enabled)
	{
		pObj.add_component(physics::physics_component{});
		pObj.add_component(physics::sprite_fixture{});
	}

	pObj.add_component(scripting::event_state_component{});

	for (auto[type, info] : util::enumerate{ events })
	{
		if (info.id.is_valid() && pAsset_mgr.has_asset(info.id))
		{
			pObj.add_component(
				scripting::event_component{ pAsset_mgr.get_asset(info.id) },
				scripting::event_descriptors[type].bucket);
		}
	}
}

json object_resource::serialize_data() const
{
	json j;
	json& jevents = j["events"];
	for (auto&& [type, info] : util::enumerate{ events })
	{
		if (info.id.is_valid())
		{
			jevents[scripting::event_descriptors[type].serialize_name] = info.id;
		}
	}
	j["sprite"] = display_sprite;
	j["is_collision_enabled"] = is_collision_enabled;
	return j;
}

void object_resource::deserialize_data(const json& pJson)
{
	const json& jevents = pJson["events"];

	for (auto&& [index, info] : util::enumerate{ events })
	{
		auto iter = jevents.find(scripting::event_descriptors[index].serialize_name);
		if (iter != jevents.end())
		{
			info.id = iter->get<asset_id>();
		}
	}

	display_sprite = pJson["sprite"];
	is_collision_enabled = pJson["is_collision_enabled"];
}

} // namespace wge::core
