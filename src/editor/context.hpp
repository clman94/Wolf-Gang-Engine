#pragma once

#include <functional>
#include <wge/logging/log.hpp>

#include "asset_document.hpp"

namespace wge::editor
{

struct context
{
private:
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

	void set_selection(core::asset::ptr pAsset)
	{

	}



private:
	std::map<std::string, editor_builder_function> mEditor_builders;
	std::vector<asset_document> mDocuments;
};

} // namespace wge::editor
