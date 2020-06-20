#include <wge/core/asset.hpp>
#include <wge/core/object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/core/scene.hpp>
#include <wge/graphics/tileset.hpp>
#include <wge/core/tilemap.hpp>

#include <vector>
#include <variant>

namespace wge::core
{

class instance
{
public:
	std::string name;
	math::transform transform;
	util::uuid asset_id;

	void from(const object& pObject);
	void generate(core::object pObject, const core::asset_manager& pAsset_mgr) const;

	static json serialize(const instance&);
	static instance deserialize(const json&);
};

struct instance_layer
{
	std::string name;
	std::vector<instance> instances;

	static constexpr const char* strtype = "instance";

	void from(core::layer&);
	void generate(core::layer& pLayer, const core::asset_manager& pAsset_mgr) const;

	static json serialize(const instance_layer&);
	static instance_layer deserialize(const json&);
};

struct tilemap_layer
{
	std::string name;
	std::vector<tile> tiles;
	math::vec2 tile_size;
	util::uuid tileset_id;

	static constexpr const char* strtype = "tilemap";

	void from(core::layer&);
	void generate(core::layer& pLayer, const core::asset_manager& pAsset_mgr) const;

	static json serialize(const tilemap_layer&);
	static tilemap_layer deserialize(const json&);
};

class scene_resource :
	public resource
{
public:
	using layer_variant = std::variant<instance_layer, tilemap_layer>;
	std::vector<layer_variant> layers;

public:
	/* Generates a json with the following format */
	/*
	
	[
		{
			type: "tilemap",
			tiles: [ { position:..., uv:... } ],
			texture:...
		}
		{
			type: "instance",
			instances: [ { name:..., transform:..., asset:... } ]
		}
	]

	*/
	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

	void generate_scene(scene& pScene, const asset_manager& pAsset_mgr) const;

	void update_data(scene& pScene);
};

} // namespace wge::core
