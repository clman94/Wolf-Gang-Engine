#pragma once

#include <functional>

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/game_object.hpp>
#include "signal.hpp"
#include <optional>

#include "asset_document.hpp"

namespace wge::editor
{

enum class selection_type
{
	asset,
	layer,
	game_object,
};

class context
{
public:
	void add_modified_asset(const core::asset::ptr& pAsset)
	{
		if (!pAsset)
			return;
		if (std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) == mUnsaved_assets.end())
			mUnsaved_assets.push_back(pAsset);
	}

	bool is_asset_modified(const core::asset::ptr& pAsset) const
	{
		return std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) != mUnsaved_assets.end();
	}

	void save_asset(const core::asset::ptr& pAsset)
	{
		auto iter = std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset);
		if (iter == mUnsaved_assets.end())
			return;
		(*iter)->get_config()->save();
		mUnsaved_assets.erase(iter);
	}

	void save_all_assets()
	{
		for (auto& i : mUnsaved_assets)
			i->get_config()->save();
		mUnsaved_assets.clear();
	}

	auto& get_unsaved_assets() noexcept
	{
		return mUnsaved_assets;
	}

	template<selection_type Ttype>
	auto get_selection() const noexcept
	{
		if constexpr (Ttype == selection_type::asset)
			return mAsset_selection.lock();
		if constexpr (Ttype == selection_type::layer)
			return mLayer_selection.lock();
		if constexpr (Ttype == selection_type::game_object)
			return mObject_selection;
	}

	// Set the currently selected asset
	void set_selection(core::asset::ptr pAsset)
	{
		reset_selection();
		mAsset_selection = pAsset;
		on_new_selection();
	}
	// Set the currently selected layer
	void set_selection(core::layer::ptr pLayer)
	{
		reset_selection();
		mLayer_selection = pLayer;
		on_new_selection();
	}
	// Set the currently selected object
	void set_selection(core::game_object pObject)
	{
		reset_selection();
		mObject_selection = pObject;
		on_new_selection();
	}

	void reset_selection()
	{
		mAsset_selection.reset();
		mLayer_selection.reset();
		mObject_selection.reset();
		on_new_selection();
	}

	util::signal<void()> on_new_selection;

private:
	core::asset::wptr mAsset_selection;
	core::layer::wptr mLayer_selection;
	std::optional<core::game_object> mObject_selection;

	std::vector<core::asset::ptr> mUnsaved_assets;
};

} // namespace wge::editor
