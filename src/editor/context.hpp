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
#include <list>

namespace wge::editor
{

class context;

class asset_editor
{
public:
	using uptr = std::unique_ptr<asset_editor>;

	asset_editor(context& pContext, const core::asset::ptr& pAsset) noexcept;
	virtual ~asset_editor() {}

	virtual void on_gui() = 0;
	virtual void on_save() {}
	virtual void on_close() {}

	const core::asset::ptr& get_asset() const noexcept
	{
		return mAsset;
	}

	context& get_context() const noexcept
	{
		return *mContext;
	}

	void focus_window() const noexcept;

	void mark_asset_modified() const;

	void set_dock_family_id(unsigned int id) { mDock_family_id = id; }
	unsigned int get_dock_family_id() { return mDock_family_id; }
	void set_dock(unsigned int pId);
	const std::string& get_window_str_id() const noexcept;

	bool is_visible() const noexcept { return mIs_visible; }
	void set_visible(bool pVisible) noexcept { mIs_visible = pVisible; }

	void set_parent_editor_id(const util::uuid& pAsset_id) noexcept { mParent_editor_asset_id = pAsset_id; }
	const util::uuid& get_parent_editor_id() const noexcept { return mParent_editor_asset_id; }

	void mark_first_time() noexcept
	{
		mFirst_time = false;
	}

	bool is_first_time() const noexcept
	{
		return mFirst_time;
	}

	core::asset_manager& get_asset_manager() const noexcept;

private:
	std::string mWindow_str_id;
	context* mContext;
	core::asset::ptr mAsset;
	bool mIs_visible = true;
	bool mFirst_time = true;
	unsigned int mDock_family_id{ 0 };

	util::uuid mParent_editor_asset_id;
};

class context
{
private:
	using editor_factory = std::function<asset_editor::uptr(const core::asset::ptr&)>;

public:
	using modified_assets = std::vector<core::asset::ptr>;

	context() = default;
	context(const context&) = delete;
	context(context&&) = delete;

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
	asset_editor* open_editor(const core::asset::ptr& pAsset, unsigned int pDock_id = 0);
	void close_editor(const core::asset::ptr& pAsset);
	void close_editor(const util::uuid& pIds);
	void close_all_editors();
	bool draw_editor(asset_editor& pEditor);
	void show_editor_guis();
	bool is_editor_open_for(const core::asset::ptr& pAsset) const;
	bool is_editor_open_for(const util::uuid& pAsset_id) const;
	asset_editor* get_editor(const core::asset::ptr& pAsset) const;
	asset_editor* get_editor(const util::uuid& pId) const;

	void set_default_dock_id(unsigned int pId) noexcept
	{
		mDefault_dock_id = pId;
	}

	unsigned int get_default_dock_id() const noexcept
	{
		return mDefault_dock_id;
	}

private:
	void erase_deleted_editors();

private:
	// This is the imgui dock id for the dockspace
	// that newly opened editors will spawn in.
	unsigned int mDefault_dock_id = 0;

	core::engine mEngine;

	// Any assets that are changed need to be added here.
	modified_assets mUnsaved_assets;
	std::map<std::string, editor_factory> mEditor_factories;

	// We don't want iterators to be invalidated while we are
	// adding and removing editors so lets just store the editors
	// in a linked list.
	std::list<asset_editor::uptr> mAsset_editors;
};

template<typename T, typename...Targs>
inline void context::register_editor(const std::string& pAsset_type, Targs&&...pExtra_args)
{
	mEditor_factories[pAsset_type] =
		// This uses a tuple to help capture lvalues as references and copy rvalues.
		[this, pExtra_args = std::tuple<Targs...>(std::forward<Targs>(pExtra_args)...)](const core::asset::ptr& pAsset)
			-> asset_editor::uptr
	{
		auto args = std::tuple_cat(std::tie(*this, pAsset), pExtra_args);
		auto make_unique_wrapper = [](auto&...pArgs) { return std::make_unique<T>(pArgs...); };
		return std::apply(make_unique_wrapper, args);
	};
}

} // namespace wge::editor
