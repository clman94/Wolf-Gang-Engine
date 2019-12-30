#include <wge/core/asset.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/core/scene.hpp>

#include <vector>
#include <variant>

namespace wge::core
{

struct tile
{
	math::ivec2 position, uv;
};

struct tilemap_info
{
	core::resource_handle<graphics::texture> texture;
};

class scene_resource :
	public resource
{
public:
	struct instance
	{
		// Unique name of instance.
		std::string name;
		math::transform transform;
		object_id id;
		util::uuid asset_id;
	};

	struct object_layer
	{
		std::string name;
		std::vector<instance> instances;
	};

	struct tilemap_layer
	{
		std::string name;
		std::vector<tile> tiles;
		util::uuid tilemap_texture;
	};

	using layer_variant = std::variant<object_layer, tilemap_layer>;
	std::vector<layer_variant> layers;

	std::vector<instance> instances;

	std::vector<tile> tilemap;
	util::uuid tilemap_texture;

public:
	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

	static object generate_instance(layer& pLayer, const asset_manager& pAsset_mgr, const instance& pData);
	layer generate_layer(const asset_manager& pAsset_mgr) const;
	scene generate_scene(const asset_manager& pAsset_mgr) const;
};

} // namespace wge::core
