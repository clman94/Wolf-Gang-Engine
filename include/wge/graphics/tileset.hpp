#pragma once

#include <wge/core/asset.hpp>

namespace wge::graphics
{

class tileset :
	public core::resource
{
public:
	math::ivec2 tile_size{ 10, 10 };
	util::uuid texture_id;

public:
	virtual json serialize_data() const override
	{
		return json{
			{ "texture",  texture_id },
			{ "tilesize", tile_size }
		};
	}
	virtual void deserialize_data(const json& pJson) override
	{
		texture_id = pJson["texture"];
		tile_size = pJson["tilesize"];
	}
};

} // namespace wge::graphics
