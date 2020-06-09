#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>
#include <array>

namespace wge::core
{

class object_resource :
	public resource
{
public:
	enum class event_type
	{
		create,
		update,
		draw,
		count,
	};

	// event_type as strings.
	static constexpr std::array<const char*, 3> event_typenames = { "create", "update", "draw" };

	// Asset id for the default sprite.
	util::uuid display_sprite;

	// Lists the ids for the scripts assets for each event type.
	std::array<util::uuid, static_cast<unsigned>(event_type::count)> events;

	bool is_collision_enabled = false;

public:
	virtual void save() override {}

	void generate_object(core::object& pObj, const asset_manager& pAsset_mgr);

	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;
};

} // namespace wge::core
