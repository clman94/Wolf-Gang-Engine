#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_stl.h>
namespace wge::editor
{

class asset_document
{
public:
	asset_document(core::asset::ptr pAsset, editor::ptr pEditor) :
		mAsset(pAsset),
		mEditor(pEditor)
	{
	}

	core::asset::ptr get_asset() const
	{
		return mAsset;
	}

	editor::ptr get_editor() const
	{
		return mEditor;
	}

	void mark_dirty()
	{
		mDirty = true;
	}

	bool is_dirty() const
	{
		return mDirty;
	}

private:
	core::asset::ptr mAsset;
	editor::ptr mEditor;
	bool mDirty{ false };
};

} // namespace wge::editor
