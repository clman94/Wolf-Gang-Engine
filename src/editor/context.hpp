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
	using editor_builder_function = std::function<editor::ptr(asset_document&)>;

	void register_editor(const std::string& pAsset_type, const editor_builder_function& pFunc)
	{
		WGE_ASSERT(mEditor_builders.find(pAsset_type) == mEditor_builders.end());
		mEditor_builders[pAsset_type] = pFunc;
	}

	void start_editor(core::asset::ptr pAsset)
	{
		if (mEditor_builders.find(pAsset->get_type()) == mEditor_builders.end())
		{
			log::error() << "Could not find editor for asset of type \"" << pAsset->get_type() + "\"" << log::endm;
		}
		//mDocuments.emplace_back(pAsset, mEditor_builders[pAsset->get_type()](pAsset));
	}

	template<selection_type Ttype>
	auto get_selection() const
	{
		if constexpr (Ttype == selection_type::asset)
			return mAsset_selection.lock();
		if constexpr (Ttype == selection_type::layer)
			return mLayer_selection.lock();
		if constexpr (Ttype == selection_type::game_object)
			return mObject_selection;
	}

	// Set the currently selection asset
	void set_selection(core::asset::ptr pAsset)
	{
		reset_selection();
		mAsset_selection = pAsset;
		on_new_selection();
	}
	// Set the currently selection layer
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

	std::map<std::string, editor_builder_function> mEditor_builders;
	std::vector<asset_document> mDocuments;
};

} // namespace wge::editor
