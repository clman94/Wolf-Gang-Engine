#pragma once

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/game_object.hpp>

#include "signal.hpp"

#include <functional>
#include <optional>

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
	using modified_asset_list = std::vector<core::asset::ptr>;

	void add_modified_asset(const core::asset::ptr& pAsset);
	void mark_selection_as_modified();
	bool is_asset_modified(const core::asset::ptr& pAsset) const;
	void save_asset(const core::asset::ptr& pAsset);
	void save_all_assets();
	modified_asset_list& get_unsaved_assets() noexcept;

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
	void set_selection(const core::asset::ptr& pAsset)
	{
		reset_selection();
		mAsset_selection = pAsset;
		on_new_selection();
	}
	// Set the currently selected layer
	void set_selection(const core::layer::ptr& pLayer)
	{
		reset_selection();
		mLayer_selection = pLayer;
		on_new_selection();
	}
	// Set the currently selected object
	void set_selection(const core::game_object& pObject)
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

	modified_asset_list mUnsaved_assets;
};

} // namespace wge::editor
