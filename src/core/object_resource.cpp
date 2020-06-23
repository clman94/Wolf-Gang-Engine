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
		graphics::sprite_component sprite;
		sprite.set_sprite(pAsset_mgr.get_asset(display_sprite));
		pObj.add_component(std::move(sprite));
	}

	if (is_collision_enabled)
	{
		pObj.add_component(physics::physics_component{});
		pObj.add_component(physics::sprite_fixture{});
	}

	if (!events.empty())
	{
		pObj.add_component(scripting::event_state_component{});

		for (auto[type, info] : util::enumerate{ events })
		{
			if (info.handle.is_valid())
			{
				scripting::event_component* comp = nullptr;
				switch (static_cast<event_type>(type))
				{
				case event_type::create:
					comp = pObj.add_component<scripting::event_selector::create>();
					break;
				case event_type::update:
					comp = pObj.add_component<scripting::event_selector::update>();
					break;
				case event_type::draw:
					comp = pObj.add_component<scripting::event_selector::draw>();
					break;
				}
				assert(comp);
				comp->source_script = info.handle;
			}
		}
	}
}

json object_resource::serialize_data() const
{
	json j;
	json& jevents = j["events"];
	for (auto[type, info] : util::enumerate{ events })
	{
		json jevent;
		const char* event_name = event_typenames[type];
		jevent["type"] = event_name;
		jevent["id"] = info.id;
		jevents.push_back(jevent);
	}
	j["sprite"] = display_sprite;
	j["is_collision_enabled"] = is_collision_enabled;
	return j;
}

void object_resource::deserialize_data(const json& pJson)
{
	const json& jevents = pJson["events"];
	for (const auto& i : jevents)
	{
		// Find the index for this event name.
		for (auto[index, name] : util::enumerate{ event_typenames })
		{
			if (name == i["type"])
			{
				// Assign the id to that index.
				events[index].id = i["id"];
				break;
			}
		}
	}
	display_sprite = pJson["sprite"];
	is_collision_enabled = pJson["is_collision_enabled"];
}

void create_object_script_assets(object_resource::handle pHandle, core::asset_manager& pAsset_mgr)
{
	const auto primary_location = std::dynamic_pointer_cast<primary_asset_location>(pHandle.get_asset()->get_location());
	std::size_t event_index = 0;
	for (auto& i : pHandle->events)
	{
		const auto name = object_resource::event_typenames[event_index];
		if (i.id.is_valid() && !pAsset_mgr.has_asset(i.id))
		{
			const auto new_asset = std::make_shared<asset>();
			new_asset->set_name(name);
			new_asset->set_id(i.id);
			new_asset->set_parent_id(pHandle.get_id());
			i.handle = new_asset;
			pAsset_mgr.add_asset(new_asset);

			auto script_resource = std::make_unique<scripting::script>();
			script_resource->set_location(secondary_asset_location::create(primary_location, name));
			script_resource->load();
			new_asset->set_resource(std::move(script_resource));
		}
		++event_index;
	}
}

} // namespace wge::core
