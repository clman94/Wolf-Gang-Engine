#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/object.hpp>
#include <wge/scripting/script.hpp>
#include <Wge/scripting/events.hpp>
#include <array>

namespace wge::core
{

class object_resource :
	public resource
{
public:
	using handle = core::resource_handle<object_resource>;

	struct event_info
	{
		asset_id id;
		scripting::script::handle handle;
	};

	// Asset id for the default sprite.
	asset_id display_sprite;

	std::array<event_info, scripting::event_count> events;

	bool is_collision_enabled = false;

public:
	virtual void save() override {}

	void generate_object(core::object& pObj, const asset_manager& pAsset_mgr);

	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;
};

} // namespace wge::core
