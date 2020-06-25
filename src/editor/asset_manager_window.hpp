#include "context.hpp"
#include <wge/core/asset_manager.hpp>

namespace wge::editor
{

class asset_manager_window
{
public:
	asset_manager_window(context& pContext, core::asset_manager& pAsset_manager);

	void on_gui();

private:
	void show_asset_directory_tree(core::asset::ptr& pCurrent_folder, const core::asset::ptr& pAsset = {});
	void asset_tile(const core::asset::ptr& pAsset, const math::vec2& pSize);
	void asset_context_menu();
	void folder_dragdrop_target(const core::asset::ptr& pAsset);

	void remove_queued_assets();

	void set_selection(const core::asset::ptr& pAsset);
	bool has_selection() const noexcept;
	bool is_singlular_selection() const;
	bool is_multi_selection() const;
	bool is_selected(const core::asset::ptr& pAsset) const;
	void select_all();
	void select(const core::asset::ptr& pAsset);
	void select_range(const core::asset::ptr& pFirst, const core::asset::ptr pLast);
	void add_selection(const core::asset::ptr& pAsset);
	void deselect();
	void deselect(const core::asset::ptr& pAsset);
	void delete_selected();

private:
	core::asset::ptr mCurrent_folder;
	context& mContext;
	core::asset_manager& mAsset_manager;
	core::asset::ptr mMulti_last_select;
	std::vector<core::asset::ptr> mSelected_asset;
	std::vector<core::asset::ptr> mRemove_queue;
};

} // namespace wge::editor
