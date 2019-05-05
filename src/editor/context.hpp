#pragma once

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/engine.hpp>

#include <wge/util/signal.hpp>

#include <functional>
#include <optional>
#include <vector>
#include <memory>
#include <tuple>

namespace wge::editor
{

class context;

class asset_editor
{
public:
	using uptr = std::unique_ptr<asset_editor>;

	asset_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
		mContext(pContext),
		mAsset(pAsset)
	{}
	virtual ~asset_editor() {}

	virtual void on_gui() = 0;

	const core::asset::ptr& get_asset() const noexcept
	{
		return mAsset;
	}

	context& get_context() const noexcept
	{
		return mContext;
	}

	void mark_asset_modified() const;

protected:
	// Reference for convenience
	context& mContext;

	core::asset::ptr mAsset;
};

class context
{
private:
	using editor_factory = std::function<asset_editor::uptr(const core::asset::ptr&)>;

public:
	using modified_assets = std::vector<core::asset::ptr>;

	void add_modified_asset(const core::asset::ptr& pAsset);
	bool is_asset_modified(const core::asset::ptr& pAsset) const;
	bool are_there_modified_assets() const noexcept;
	void save_asset(const core::asset::ptr& pAsset);
	void save_all_assets();
	modified_assets& get_unsaved_assets() noexcept;

	core::engine& get_engine() noexcept
	{
		return mEngine;
	}

	template <typename T, typename...Targs>
	void register_editor(const std::string& pAsset_type, Targs&&...);
	void open_editor(const core::asset::ptr& pAsset);
	void close_editor(const core::asset::ptr& pAsset);
	void show_editor_guis();
	bool is_editor_open_for(const core::asset::ptr& pAsset) const;

private:
	modified_assets mUnsaved_assets;
	core::engine mEngine;
	std::map<std::string, editor_factory> mEditor_factories;
	std::vector<asset_editor::uptr> mAsset_editors;
};

template<typename T, typename...Targs>
inline void context::register_editor(const std::string& pAsset_type, Targs&&...pExtra_args)
{
	mEditor_factories[pAsset_type] =
		[this, pExtra_args = std::tuple<Targs...>(std::forward<Targs>(pExtra_args)...)](const core::asset::ptr& pAsset)
			-> asset_editor::uptr
	{
		auto args = std::tuple_cat(std::tie(*this, pAsset), pExtra_args);
		auto make_unique_wrapper = [](auto&...pArgs) { return std::make_unique<T>(pArgs...); };
		return std::apply(make_unique_wrapper, args);
	};
}

} // namespace wge::editor
