#include <wge/core/object_resource.hpp>
#include <wge/scripting/lua_engine.hpp>
#include <wge/util/ipair.hpp>
#include <wge/graphics/sprite_component.hpp>

namespace wge::core
{

void object_resource::generate_object(core::game_object& pObj, const asset_manager& pAsset_mgr)
{
	pObj.add_component<core::transform_component>();

	if (display_sprite.is_valid())
	{
		auto sprite = pObj.add_component<graphics::sprite_component>();
		sprite->set_texture(pAsset_mgr.get_asset(display_sprite));
	}

	if (!events.empty())
	{
		pObj.add_component<scripting::event_state_component>();

		for (const auto&[type, asset_id] : util::ipair{ events })
		{
			scripting::event_component* comp = nullptr;
			switch (static_cast<event_type>(type))
			{
			case event_type::create:
				comp = pObj.add_component<scripting::event_components::on_create>();
				break;
			case event_type::update:
				comp = pObj.add_component<scripting::event_components::on_update>();
				break;
			}
			assert(comp);
			if (auto asset = pAsset_mgr.get_asset(asset_id))
				comp->source_script = asset;
			else
				log::error() << "Could not load event script; Invalid id" << log::endm;
		}
	}
}

json object_resource::serialize_data() const
{
	json j;
	json& jevents = j["events"];
	for (const auto&[type, asset_id] : util::ipair{ events })
	{
		json jevent;
		const char* event_name = event_typenames[type];
		jevent["type"] = event_name;
		jevent["id"] = asset_id;
		jevents.push_back(jevent);
	}
	j["sprite"] = display_sprite;

	return j;
}

void object_resource::deserialize_data(const json& pJson)
{
	const json& jevents = pJson["events"];
	for (const auto& i : jevents)
	{
		// Find the index for this event name.
		for (auto[index, name] : util::ipair{ event_typenames })
		{
			if (name == i["type"])
			{
				// Assign the id to that index.
				events[index] = i["id"];
				break;
			}
		}
	}

	display_sprite = pJson["sprite"];
}
} // namespace wge::core
