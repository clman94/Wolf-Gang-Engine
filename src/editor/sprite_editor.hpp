#pragma once

#include "context.hpp"
#include <wge/graphics/sprite.hpp>

namespace wge::editor
{

class sprite_editor :
	public asset_editor
{
public:
	sprite_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset),
		mController(pAsset)
	{}
	void on_gui();

private:
	void check_if_edited();

private:
	graphics::sprite_controller mController;
	float mAtlas_info_width = 200;
};

} // namespace wge::editor
