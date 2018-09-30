#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/component.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/math/anchor.hpp>

namespace wge::graphics
{

class sprite_component :
	public core::component
{
	WGE_COMPONENT_SINGLE_INSTANCE("Sprite", 12409);
public:
	sprite_component(core::object_node* pNode);

	virtual json serialize() const override;
	virtual void deserialize(const json& pJson) override;

	// Creates a batch and sends it to the renderer
	void on_render(renderer* pRenderer);

	// Set the offset of the image in pixels
	void set_offset(const math::vec2& pOffset)
	{
		mOffset = pOffset;
	}
	// Get the offset of the image in pixels
	math::vec2 get_offset() const
	{
		return mOffset;
	}

	// Set the anchor ratio. Defaults topleft (0, 0).
	void set_anchor(const math::vec2& pRatio)
	{
		mAnchor = pRatio;
	}
	math::vec2 get_anchor() const
	{
		return mAnchor;
	}

	// Set the texture from a texture pointer.
	void set_texture(texture::ptr pAsset)
	{
		mTexture = pAsset;
	}
	// Set the texture based on its asset id.
	// This will use the asset manager in the current context.
	void set_texture(core::asset_uid pID);
	// Set the texture based on its asset path.
	// This will use the asset manager in the current context.
	void set_texture(const std::string& pPath);
	// Get the current texture
	texture::ptr get_texture() const
	{
		return mTexture;
	}

private:
	texture::ptr mTexture;
	math::vec2 mOffset, mAnchor{ math::anchor::topleft };
};

} // namespace wge::graphics