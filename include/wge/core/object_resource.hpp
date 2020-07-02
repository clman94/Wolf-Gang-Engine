#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/object.hpp>
#include <wge/scripting/script.hpp>
#include <array>

namespace wge::core
{

class object_resource :
	public resource
{
public:
	using handle = core::resource_handle<object_resource>;

	enum class event_type
	{
		create,
		update,
		draw,
		count,
	};

	struct event_info
	{
		util::uuid id;
		scripting::script::handle handle;
	};

	// event_type as strings.
	static constexpr std::array<const char*, 3> event_typenames = { "create", "update", "draw" };

	// Asset id for the default sprite.
	util::uuid display_sprite;

	std::array<event_info, static_cast<unsigned>(event_type::count)> events;

	bool is_collision_enabled = false;

public:
	virtual void save() override {}

	void generate_object(core::object& pObj, const asset_manager& pAsset_mgr);

	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

	static void load_event_scripts(const object_resource::handle& pHandle, core::asset_manager& pAsset_mgr);
};

} // namespace wge::core
