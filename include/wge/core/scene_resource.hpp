#pragma once

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
#include <unordered_set>
#include <variant>

namespace wge::core
{

class scene_resource :
	public resource
{
public:
	using handle = core::resource_handle<scene_resource>;

	// Raw scene data. Should not be modified directly under normal circumstances.
	// Use generate_scene() to create a scene based on this data
	// then call update_data to convert it back.
	json scene_data{ { "layers", json::array() } };

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
