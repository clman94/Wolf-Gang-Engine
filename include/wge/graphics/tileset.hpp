#pragma once

#include <wge/core/asset.hpp>
#include <wge/graphics/texture.hpp>

namespace wge::graphics
{

class tileset :
	public core::resource
{
public:
	math::ivec2 tile_size{ 10, 10 };

public:
	virtual void load() override
	{
		image tsimage;
		tsimage.load_file(get_location().get_autonamed_file(".png").string());
		mTexture.set_image(std::move(tsimage));
	}

	virtual json serialize_data() const override
	{
		return json{
			{ "tile_size",  tile_size }
		};
	}

	virtual void deserialize_data(const json& pJson) override
	{
		tile_size = pJson["tile_size"];
	}

	void set_texture_implementation(texture_impl::ptr pImpl)
	{
		mTexture.set_implementation(pImpl);
	}

	const texture& get_texture() const noexcept
	{
		return mTexture;
	}

private:
	texture mTexture;
};

} // namespace wge::graphics
