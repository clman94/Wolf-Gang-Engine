#include <rpg/editor.hpp>

using namespace editors;

#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include <cstring>

#include <engine/logger.hpp>
#include <engine/utility.hpp>
#include <engine/math.hpp>

#include <editor/file_opener.hpp>

inline engine::fvector read_args_vector(const engine::terminal_arglist& pArgs, float pDefx = 0, float pDefy = 0, size_t pIndex = 0)
{
	engine::fvector ret;
	if (pArgs[pIndex].get_raw() == "-")
		ret.x = pDefx;
	else
		ret.x = util::to_numeral<float>(pArgs[pIndex]);

	if (pArgs[pIndex + 1].get_raw() == "-")
		ret.y = pDefy;
	else
		ret.y = util::to_numeral<float>(pArgs[pIndex + 1]);
	return ret;
}

bool command_manager::add(std::shared_ptr<command> pCommand)
{
	assert(!mCurrent);
	mRedo.clear();
	mUndo.push_back(pCommand);
	return true;
}

void command_manager::complete()
{
	assert(mCurrent);
	mRedo.clear();
	mUndo.push_back(mCurrent);
	mCurrent.reset();
}

bool command_manager::execute_and_complete()
{
	mRedo.clear();
	mUndo.push_back(mCurrent);
	bool r = mCurrent->execute();
	mCurrent.reset();
	return r;
}

bool command_manager::undo()
{
	assert(!mCurrent);
	if (mUndo.size() == 0)
		return false;
	auto undo_cmd = mUndo.back();
	mUndo.pop_back();
	mRedo.push_back(undo_cmd);
	return undo_cmd->undo();
}

bool command_manager::redo()
{
	assert(!mCurrent);
	if (mRedo.size() == 0)
		return false;
	auto redo_cmd = mRedo.back();
	mRedo.pop_back();
	mUndo.push_back(redo_cmd);
	return redo_cmd->redo();
}

void command_manager::clear()
{
	mUndo.clear();
	mRedo.clear();
	mCurrent.reset();
}

class command_set_tiles :
	public command
{
public:
	command_set_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
	}

	command_set_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator, rpg::tile& pTile)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
		add(pTile);
	}

	bool execute()
	{
		assert(mTilemap_manipulator);

		// Track all the tiles that are replaced
		for (auto& i : mTiles)
		{
			auto replaced_tile = mTilemap_manipulator->get_layer(mLayer).find_tile(i.get_position());
			if (replaced_tile)
				mReplaced_tiles.push_back(*replaced_tile);
		}

		// Replace all the tiles
		for (auto& i : mTiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);

		return true;
	}

	bool undo()
	{
		assert(mTilemap_manipulator);

		// Remove all placed tiles
		for (auto& i : mTiles)
			mTilemap_manipulator->get_layer(mLayer).remove_tile(i.get_position());

		// Place all replaced tiles back
		for (auto& i : mReplaced_tiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);

		return true;
	}

	void add(rpg::tile& pTile)
	{
		mTiles.push_back(pTile);
	}

private:
	std::size_t mLayer;

	std::vector<rpg::tile> mReplaced_tiles;
	std::vector<rpg::tile> mTiles;

	rpg::tilemap_manipulator* mTilemap_manipulator;
};

class command_remove_tiles :
	public command
{
public:
	command_remove_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
	}

	command_remove_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator, engine::fvector pTile)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
		add(pTile);
	}
	bool execute()
	{
		assert(mTilemap_manipulator);

		// Track all the tiles that are replaced
		for (auto& i : mTiles_to_remove)
		{
			auto replaced_tile = mTilemap_manipulator->get_layer(mLayer).find_tile(i);
			if (replaced_tile)
			{
				mRemoved_tiles.push_back(*replaced_tile);
				mTilemap_manipulator->get_layer(mLayer).remove_tile(i);
			}
		}

		return true;
	}

	bool undo()
	{
		for (auto& i : mRemoved_tiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);
		return true;
	}

	void add(engine::fvector pPosition)
	{
		mTiles_to_remove.push_back(pPosition);
	}

private:
	std::size_t mLayer;

	std::vector<rpg::tile> mRemoved_tiles;
	std::vector<engine::fvector> mTiles_to_remove;

	rpg::tilemap_manipulator* mTilemap_manipulator;
};

editor::editor()
{
	mIs_changed = false;
}

void editor::set_game(rpg::game & pGame)
{
	mGame = &pGame;
}

bool editor::is_changed() const
{
	return mIs_changed;
}

void editor::editor_changed()
{
	mIs_changed = true;
}

// ##########
// scene_editor
// ##########

scene_editor::scene_editor()
{
	mZoom = 0;
}

bool scene_editor::open_scene(std::string pName)
{
	mTilemap_manipulator.clear();
	mTilemap_display.clear();

	assert(mGame);

	engine::generic_path path((mGame->get_source_path() / "scenes" / pName).string());
	if (!mLoader.load(path.parent(), path.filename()))
	{
		logger::error("Unable to open scene '" + pName + "'");
		return false;
	}

	assert(mGame != nullptr);
	auto texture = mGame->get_resource_manager().get_resource<engine::texture>("texture", mLoader.get_tilemap_texture());
	if (!texture)
	{
		logger::warning("Invalid tilemap texture in scene");
		logger::info("If you have yet to specify a tilemap texture, you can ignore the last warning");
	}
	else
	{
		mTilemap_display.set_texture(texture);
		//mTilemap_display.set_color({ 100, 100, 255, 150 });

		mTilemap_manipulator.load_tilemap_xml(mLoader.get_tilemap());
		mTilemap_display.update(mTilemap_manipulator);
	}
	return true;
}

void scene_editor::update_zoom(engine::renderer & pR)
{
	if (pR.is_key_pressed(engine::renderer::key_code::Add))
		mZoom += 0.5;
	if (pR.is_key_pressed(engine::renderer::key_code::Subtract))
		mZoom -= 0.5;
	float factor = std::pow(2.f, mZoom);
	set_scale({ factor, factor });
}

// ##########
// collisionbox_editor
// ##########

class command_add_wall :
	public command
{
public:
	command_add_wall(std::shared_ptr<rpg::collision_box> pBox, rpg::collision_box_container* pContainer)
	{
		mBox = pBox;
		mContainer = pContainer;
	}

	bool execute()
	{
		mContainer->add_collision_box(mBox);
		return true;
	}

	bool undo()
	{
		mContainer->remove_box(mBox);
		return true;
	}

	bool redo()
	{
		return execute();
	}

private:
	std::shared_ptr<rpg::collision_box> mBox;
	rpg::collision_box_container* mContainer;
};

class command_remove_wall :
	public command
{
public:
	command_remove_wall(std::shared_ptr<rpg::collision_box> pBox, rpg::collision_box_container* pContainer)
		: pOpposing(pBox, pContainer)
	{}

	bool execute()
	{
		return pOpposing.undo();
	}

	bool undo()
	{
		return pOpposing.execute();
	}

	bool redo()
	{
		return execute();
	}

private:
	command_add_wall pOpposing;
};

class command_wall_changed :
	public command
{
public:
	command_wall_changed(std::shared_ptr<rpg::collision_box> pBox)
		: mBox(pBox), mOpposing(pBox->copy())
	{}

	bool execute()
	{
		std::shared_ptr<rpg::collision_box> temp(mBox->copy());
		mBox->set(mOpposing);
		mOpposing->set(temp);
		return true;
	}

	bool undo()
	{
		return execute();
	}

	bool redo()
	{
		return execute();
	}

private:
	std::shared_ptr<rpg::collision_box> mBox;
	std::shared_ptr<rpg::collision_box> mOpposing;
};

collisionbox_editor::collisionbox_editor()
{
	mWall_display.set_color({ 100, 255, 100, 200 });
	mWall_display.set_outline_color({ 255, 255, 255, 255 });
	mWall_display.set_outline_thinkness(1);

	mGrid.set_major_size({ 32, 32 });
	mGrid.set_sub_grids(2);

	mCurrent_type = rpg::collision_box::type::wall;
	mGrid_snap = grid_snap::full;

	mState = state::normal;
}

bool collisionbox_editor::open_editor()
{
	mCommand_manager.clear();
	mContainer.clear();
	if (mLoader.get_collisionboxes())
		return mContainer.load_xml(mLoader.get_collisionboxes());
	return true;
}

int collisionbox_editor::draw(engine::renderer& pR)
{
	update_zoom(pR);

	const bool button_left = pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left);
	const bool button_left_down = pR.is_mouse_down(engine::renderer::mouse_button::mouse_left);
	const bool button_right = pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right);
	const bool button_shift = pR.is_key_down(engine::renderer::key_code::LShift);
	const bool button_ctrl = pR.is_key_down(engine::renderer::key_code::LControl);

	const engine::fvector mouse_position = pR.get_mouse_position(*this);
	const engine::fvector exact_tile_position = (mouse_position * get_unit()).floor()/std::pow(get_unit(), 2);

	engine::fvector tile_position;
	engine::fvector selection_size;
	if (mGrid_snap == grid_snap::none) // No grid
	{
		tile_position = exact_tile_position;
		selection_size = engine::fvector(0, 0);
	}
	else
	{
		// Snap the tile position and adjust the selection size for the new grid
		float scale = 1;
		switch (mGrid_snap)
		{
		case grid_snap::pixel:   scale = 1 / get_unit(); break;
		case grid_snap::eighth:  scale = 0.25f;          break;
		case grid_snap::quarter: scale = 0.5f;           break;
		case grid_snap::full:    scale = 1;              break;
		}
		tile_position = (exact_tile_position / scale).floor() * scale;
		selection_size = engine::fvector(scale, scale);
	}

	switch (mState)
	{
	case state::normal:
	{
		// Select tile
		if (button_left)
		{
			if (button_ctrl && mSelection) // Resize
			{
				const engine::fvector pos = (exact_tile_position - mSelection->get_region().get_center())/**mSelection->get_region().get_size().normalize()*/;
				if (std::abs(pos.x) > std::abs(pos.y))
				{
					if (pos.x > 0)
						mResize_mask = engine::frect(0, 0, 1, 0); // Right
					else
						mResize_mask = engine::frect(1, 0, -1, 0); // Left
				}
				else
				{
					if (pos.y > 0)
						mResize_mask = engine::frect(0, 0, 0, 1); // Bottom
					else
						mResize_mask = engine::frect(0, 1, 0, -1); // Top
				}
				mState = state::resize_mode;
				mCommand_manager.add(std::make_shared<command_wall_changed>(mSelection));
				mOriginal_rect = mSelection->get_region();
				mDrag_from = tile_position;
			}
			else if (!tile_selection(exact_tile_position) // Create/Select
				|| button_shift) // Left shift allows us to place wall on another wall
			{
				mSelection = mContainer.add_collision_box(mCurrent_type);
				mCommand_manager.add(std::make_shared<command_add_wall>(mSelection, &mContainer));
				mSelection->set_region({ tile_position, selection_size });

				mState = state::size_mode;
				mDrag_from = tile_position;
			}
			else // Move
			{
				mCommand_manager.add(std::make_shared<command_wall_changed>(mSelection));
				mState = state::move_mode;
				mDrag_from = tile_position - mSelection->get_region().get_offset();
			}
		}

		// Remove tile
		else if (button_right)
		{
			// No cycling when removing tile.
			if (tile_selection(exact_tile_position, false))
			{
				mSelection = nullptr;
			}
		}

		else if (pR.is_key_down(engine::renderer::key_code::LControl))
		{
			if (pR.is_key_pressed(engine::renderer::key_code::Z)) // Undo
			{
				mCommand_manager.undo();
			}
			else if (pR.is_key_pressed(engine::renderer::key_code::Y)) // Redo
			{
				mCommand_manager.redo();
			}
		}

		break;
	}
	case state::size_mode:
	{
		// Size mode only last while user is holding down left-click
		if (!button_left_down)
		{
			mState = state::normal;
			break;
		}

		engine::frect   rect = mSelection->get_region();
		engine::fvector resize_to = tile_position;

		// Cursor moves behind the initial point where the wall is created
		if (tile_position.x <= mDrag_from.x)
		{
			rect.x = tile_position.x;   // Move the wall back
			resize_to.x = mDrag_from.x; // Resize accordingly
		}
		if (tile_position.y <= mDrag_from.y)
		{
			rect.y = tile_position.y;
			resize_to.y = mDrag_from.y;
		}
		rect.set_size(resize_to - rect.get_offset() + selection_size);

		// Update wall
		mSelection->set_region(rect);

		break;
	}
	case state::move_mode:
	{		
		if (!button_left_down)
		{
			mState = state::normal;
			break;
		}

		auto rect = mSelection->get_region();
		rect.set_offset(tile_position - mDrag_from);
		mSelection->set_region(rect);
		break;
	}
	case state::resize_mode:
	{
		if (!button_left_down)
		{
			mState = state::normal;
			break;
		}
		auto rect = mSelection->get_region();
		rect.set_offset(mOriginal_rect.get_offset() + (tile_position - mDrag_from)*mResize_mask.get_offset());
		rect.set_size(mOriginal_rect.get_size() + (tile_position - mDrag_from)*mResize_mask.get_size());

		// Limit size
		rect.w = std::max(rect.w, selection_size.x);
		rect.h = std::max(rect.h, selection_size.y);

		mSelection->set_region(rect);
		break;
	}
	}

	mTilemap_display.draw(pR);

	mGrid.update_grid(pR);
	mGrid.draw(pR);

	for (auto& i : mContainer.get_boxes()) // TODO: Optimize
	{
		if (i == mSelection)
			mWall_display.set_outline_color({ 180, 90, 90, 255 });   // Outline wall red if selected...
		else
			mWall_display.set_outline_color({ 255, 255, 255, 255 }); // ...Otherwise it is white

		if (!i->get_wall_group())
			mWall_display.set_color({ 100, 255, 100, 200 }); // Green wall if not in a wall group...
		else
			mWall_display.set_color({ 200, 100, 200, 200 }); // ...Purple-ish otherwise

		// The wall region has to be scaled to pixel coordinates
		mWall_display.set_position(i->get_region().get_offset());
		mWall_display.set_size(i->get_region().get_size() * get_unit());
		mWall_display.draw(pR);
	}
	return 0;
}

void collisionbox_editor::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mCollision_editor_group = std::make_shared<engine::terminal_command_group>();
	mCollision_editor_group->set_root_command("collision");
	mCollision_editor_group->add_command("clear",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mContainer.clear();
		mSelection.reset();
		return true;
	}, "- Clear all collision boxes (Warning: Can't undo)");
	pTerminal.add_group(mCollision_editor_group);
}

bool collisionbox_editor::save()
{
	logger::info("Saving collision boxes");

	mContainer.save_xml(mLoader.get_document(), mLoader.get_collisionboxes());
	mLoader.save();

	logger::info("Saved " + std::to_string(mContainer.get_count()) +" collision box(es)");

	return editor::save();
}

bool collisionbox_editor::tile_selection(engine::fvector pCursor, bool pCycle)
{
	const auto hits = mContainer.collision(pCursor);
	if (hits.empty())
	{
		mSelection = nullptr;
		return false;
	}

	// Cycle through overlapping walls.
	// Check if selection is selected again.
	if (mSelection
		&& mSelection->get_region().is_intersect(pCursor))
	{
		// It will not select the tile underneath by it will retain the current
		// selection if selected
		if (!pCycle)
			return true;

		// Find the hit that is underneath the current selection.
		// Start at 1 because it does require that there is one wall underneath
		// and overall works well when looping through.
		for (size_t i = 1; i < hits.size(); i++)
			if (hits[i] == mSelection)
			{
				mSelection = hits[i - 1];
				return true;
			}
	}

	mSelection = hits.back(); // Top hit
	return true;
}

// ##########
// atlas_editor
// ##########

atlas_editor::atlas_editor()
{
	mTexture.reset(new engine::texture);

	mFull_animation.set_color({ 100, 100, 255, 100 });
	mFull_animation.set_parent(mSprite);

	mSelected_firstframe.set_color({ 0, 0, 0, 0 });
	mSelected_firstframe.set_outline_color({ 255, 255, 0, 255 });
	mSelected_firstframe.set_outline_thinkness(1);
	mSelected_firstframe.set_parent(mSprite);
	
	mPreview_bg.set_anchor(engine::anchor::bottom);
	mPreview_bg.set_color({ 0, 0, 0, 200 });
	mPreview_bg.set_outline_color({ 255, 255, 255, 200 });
	mPreview_bg.set_outline_thinkness(1);
	mPreview_bg.set_parent(mSprite);

	mPreview.set_anchor(engine::anchor::bottom);
	mPreview.set_parent(mPreview_bg);

	mBackground.set_color({ 0, 0, 0, 255 });

	black_background();
	mZoom = 1;
	mPreview.set_visible(false);
	get_textures("./data/textures");
	if (!mTexture_list.empty())
	{
		setup_for_texture(mTexture_list[0]);
	}
}

bool atlas_editor::open_editor()
{

	return true;
}

int atlas_editor::draw(engine::renderer & pR)
{
	const bool button_left = pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left);
	const bool button_left_down = pR.is_mouse_down(engine::renderer::mouse_button::mouse_left);
	const bool button_right = pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right);
	const bool button_shift = pR.is_key_down(engine::renderer::key_code::LShift);
	const bool button_ctrl = pR.is_key_down(engine::renderer::key_code::LControl);

	const engine::fvector mouse_position = pR.get_mouse_position();

	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right))
		mDrag_offset = mSprite.get_position() - mouse_position;
	else if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_right))
		mSprite.set_position(mouse_position + mDrag_offset);

	if (pR.is_key_pressed(engine::renderer::key_code::Add))
	{
		mZoom += 1;
		mSprite.set_scale({ mZoom, mZoom });
		update_preview();
	}
	
	if (pR.is_key_pressed(engine::renderer::key_code::Subtract))
	{
		mZoom -= 1;
		mSprite.set_scale({ mZoom, mZoom });
		update_preview();
	}

	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left))
		atlas_selection((mouse_position - mSprite.get_position())/mZoom);

	mSprite.draw(pR);

	for (auto& i : mAtlas.get_all())
	{
		auto full_region = i->get_full_region()*mZoom;
		if (i == mSelection)
			mFull_animation.set_color({ 1, 1, 0.39f, 0.19f });
		else
			mFull_animation.set_color({ 0.39f, 0.39f, 1, 0.39f });
		mFull_animation.set_position(full_region.get_offset());
		mFull_animation.set_size(full_region.get_size());
		mFull_animation.draw(pR);

		if (i == mSelection)
		{
			auto rect = i->get_frame_at(i->get_default_frame())*mZoom;
			mSelected_firstframe.set_position(rect.get_offset());
			mSelected_firstframe.set_size(rect.get_size());
			mSelected_firstframe.draw(pR);
		}
	}

	// Animation Preview
	if (mSelection && mSelection->get_frame_count() > 1) // Only display if there is an animation
	{
		mPreview_bg.draw(pR);
		mPreview.draw(pR);
	}

	return 0;
}

bool atlas_editor::save()
{
	if (mTexture_list.empty())
		return false;
	const std::string xml_path = mLoaded_texture.string() + ".xml";
	logger::info("Saving atlas '" + xml_path + "'...");
	
	mAtlas.remove_entry("_name_here_");
	mAtlas.save(xml_path);
	mAtlas_changed = false;
	logger::info("Atlas save");

	return editor::save();
}

void atlas_editor::get_textures(const std::string & pPath)
{
	mTexture_list.clear();
	for (auto& i : engine::fs::recursive_directory_iterator(pPath))
	{
		engine::generic_path path = i.path().string();
		if (path.extension() == ".png")
		{
			mTexture_list.push_back(path.parent() / path.stem());

			if (engine::fs::exists(i.path().parent_path() / (i.path().stem().string() + ".xml")))
			{ }
		}
	}
}

void atlas_editor::setup_for_texture(const engine::generic_path& pPath)
{
	mAtlas_changed = false;
	mLoaded_texture = pPath;

	const std::string texture_path = pPath.string() + ".png";
	mTexture->unload();
	mTexture->set_texture_source(texture_path);
	mTexture->load();
	mPreview.set_texture(mTexture);
	mSprite.set_texture(mTexture);
	mSprite.set_texture_rect({ engine::fvector(0, 0), mTexture->get_size() });

	mSelection = nullptr;
	mAtlas.clear();

	const std::string xml_path = pPath.string() + ".xml";
	if (!engine::fs::exists(xml_path))
	{
		logger::info("Starting a new atlas");
		new_entry();
		return;
	}

	mAtlas.load(pPath.string() + ".xml");
}

void atlas_editor::new_entry()
{
	if (auto find = mAtlas.get_entry("_Name_here_"))
	{
		logger::warning("A new, unnamed, entry has already been created");
		mSelection = find;
		update_preview();
		return;
	}
	mSelection = std::make_shared<engine::subtexture>();
	mSelection->set_name("_Name_here_");
	mSelection->set_frame_count(1);
	mSelection->set_loop(engine::animation::loop_type::none);
	mAtlas.add_entry(mSelection);
	update_preview();
	mAtlas_changed = true;
}

void atlas_editor::remove_selected()
{
	mAtlas.remove_entry(mSelection);
	if (mAtlas.is_empty())
		mSelection = nullptr;
	else
		mSelection = mAtlas.get_all().back();
	update_preview();
	mAtlas_changed = true;
}

void atlas_editor::atlas_selection(engine::fvector pPosition)
{
	std::vector<engine::subtexture::ptr> hits;
	for (auto& i : mAtlas.get_all())
		if (i->get_full_region().is_intersect(pPosition))
			hits.push_back(i);

	if (hits.empty())
		return;

	// Similar cycling as the collisionbox editor
	for (size_t i = 1; i < hits.size(); i++)
	{
		if (hits[i] == mSelection)
		{
			mSelection = hits[i - 1];
			update_preview();
			return;
		}
	}
	mSelection = hits.back();
	update_preview();
}

void atlas_editor::black_background()
{
	mPreview_bg.set_color({ 0, 0, 0, 0.58f });
	mPreview_bg.set_outline_color({ 1, 1, 1, 0.78f });
}

void atlas_editor::white_background()
{
	mPreview_bg.set_color({ 255, 255, 255, 0.58f });
	mPreview_bg.set_outline_color({ 0, 0, 0, 0.78f });
}

void atlas_editor::update_preview()
{
	if (!mSelection)
		return;

	auto position = mSelection->get_frame_at(0).get_offset();
	position.x += mSelection->get_full_region().w / 2;
	mPreview_bg.set_position(position);

	mPreview.set_scale({ mZoom, mZoom });
	mPreview.set_animation(mSelection);
	mPreview.restart();
	mPreview.start();

	mPreview_bg.set_size(mPreview.get_size());

}



bool editor_settings_loader::load(const engine::fs::path& pPath)
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(pPath.string().c_str()) != tinyxml2::XML_SUCCESS)
		return false;
	auto ele_root = doc.RootElement();
	mPath = util::safe_string(ele_root->FirstChildElement("path")->GetText());
	mOpen_param = util::safe_string(ele_root->FirstChildElement("open")->GetText());
	mOpento_param = util::safe_string(ele_root->FirstChildElement("opento")->GetText());
	return true;
}

std::string editor_settings_loader::generate_open_cmd(const std::string & pFilepath) const
{
	std::string modified_param = mOpen_param;
	util::replace_all_with(modified_param, "%filename%", pFilepath);
	return "\"" + mPath + "\" " + modified_param;
}

std::string editor_settings_loader::generate_opento_cmd(const std::string & pFilepath, size_t pRow, size_t pCol)
{
	std::string modified_param = mOpento_param;
	util::replace_all_with(modified_param, "%filename%", pFilepath);
	util::replace_all_with(modified_param, "%row%", std::to_string(pRow));
	util::replace_all_with(modified_param, "%col%", std::to_string(pCol));
	return "\"" + mPath + "\" " + modified_param;
}



/* 
###Menu
Game
Button open game - Open a data folder
Button New game - Create a new data folder with game

Tools


###Windows

#Game
Inputtext Name


#Scene
Vec4 Boundary - set Boundary of scene
Button Rename - Rename the scene
Treelist (of file directories) or just a combo box to choose scene

#Tilemap

#Tilemap Display

#Collision

#Collision Display

#Terminal
Displays

#Script
Suspend/resume - Suspend or resume scripts
Callstack - Display callstack if suspended or on some sort of error
Compile messages

*/

namespace ImGui
{
static inline void AddBackgroundImage(sf::RenderTexture& pRender)
{
	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	engine::frect box(ImGui::GetCursorScreenPos()
		, engine::vector_cast<float, unsigned int>(pRender.getSize()) + ImGui::GetCursorPos());
	drawlist->AddImage((void*)pRender.getTexture().getNativeHandle()
		, box.get_offset(), box.get_corner()
		, ImVec2(0, 1), ImVec2(1, 0) // Render textures store textures upsidedown so we need to flip it
		, ImGui::GetColorU32(sf::Color::White));
}

static inline void VSplitter(const char* str_id, float width, float* val)
{
	assert(val);
	ImVec2 last_item_min = ImGui::GetItemRectMin();
	ImVec2 last_item_max = ImGui::GetItemRectMax();
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::InvisibleButton(str_id, ImVec2(width, last_item_max.y - last_item_min.y));
	if (ImGui::IsItemActive())
		*val += ImGui::GetIO().MouseDelta.x;
	ImGui::PopStyleVar();
}

// Construct a string representing the name and id "name##id"
static inline std::string NameId(const std::string& pName, const std::string& pId)
{
	return pName + "###" + pId;
}

static inline std::string IdOnly(const std::string& pId)
{
	return "###" + pId;
}

// Shortcut for adding a text only tooltip to the last item
static inline void QuickTooltip(const char * pString)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.f);
		ImGui::TextUnformatted(pString);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// Returns 1 if the user pressed "Yes" and 2 if "No"
static inline int ConfirmPopup(const char * pName, const char* pMessage)
{
	int answer = 0;
	if (ImGui::BeginPopupModal(pName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(pMessage);
		if (ImGui::Button("Yes", ImVec2(100, 25)))
		{
			answer = 1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100, 25)))
		{
			answer = 2;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	return answer;
}

static inline bool TextureSelectCombo(const char* pName, const engine::resource_manager& pRes_mgr, std::string* pTexture_name)
{
	assert(pTexture_name);
	bool ret = false;
	if (ImGui::BeginCombo(pName, pTexture_name->empty() ? "No Texture" : pTexture_name->c_str()))
	{
		for (auto& i : pRes_mgr.get_resources_with_type(engine::texture_restype))
		{
			if (ImGui::Selectable(i->get_name().c_str(), *pTexture_name == i->get_name()))
			{
				*pTexture_name = i->get_name();
				ret = true;
			}
		}
		ImGui::EndCombo();
	}
	return ret;
}

}

int OSA_distance(const std::string& pStr1, const std::string& pStr2)
{
	std::vector<std::vector<int>> d;
	d.resize(pStr1.length() + 1);
	for (std::size_t i = 0; i < d.size(); i++)
		d[i].resize(pStr2.length() + 1);

	for (std::size_t i = 0; i < pStr1.length() + 1; i++)
		d[i][0] = i;
	for (std::size_t i = 0; i < pStr2.length() + 1; i++)
		d[0][i] = i;

	for (std::size_t i = 1; i < pStr1.length() + 1; i++)
	{
		for (std::size_t j = 1; j < pStr2.length() + 1; j++)
		{
			std::size_t str1_i = i - 1;
			std::size_t str2_i = j - 1;
			int cost = (pStr1[str1_i] == pStr2[str2_i]) ? 0 : 1;
			d[i][j] = std::min({
					d[i - 1][j] + 1,
					d[i][j - 1] + 1,
					d[i - 1][j - 1] + cost
				});
			if (i > 1 && j > 1 && pStr1[str1_i] == pStr2[str2_i - 1] && pStr1[str1_i - 1] == pStr2[str2_i])
				d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + cost);
		}
	}
	return d[pStr1.length()][pStr2.length()];
}

engine::fvector snap(const engine::fvector& pPos, const engine::fvector& pTo, const engine::fvector& pOffset = {0, 0})
{
	if (pTo.has_zero())
		return pPos;
	return (pPos / pTo - pOffset).floor() * pTo + pOffset;
}

void draw_grid(engine::primitive_builder& pPrimitives, engine::fvector pAlign_to, engine::fvector pScale, engine::fvector pDisplay_size, engine::color pColor)
{
	engine::ivector line_count = engine::vector_cast<int>((pDisplay_size / pScale).floor());
	engine::fvector offset = math::pfmodf(pAlign_to, pScale);

	// Vertical lines
	for (int i = 0; i < line_count.x; i++)
	{
		float x = (float)i*pScale.x + offset.x;
		if (x < 0)
			x += pDisplay_size.x;
		pPrimitives.add_line({ x, 0 }, { x, pDisplay_size.y }, pColor);
	}

	// Horizontal lines
	for (int i = 0; i < line_count.y; i++)
	{
		float y = (float)i*pScale.y + offset.y;
		if (y < 0)
			y += pDisplay_size.y;
		pPrimitives.add_line({ 0, y }, { pDisplay_size.x, y }, pColor);
	}
}

// Resizes a render texture if the imgui window was changed size.
// Works best if this is the first thing drawn in the window.
// Returns true if the texture was actually changed.
static inline bool resize_to_window(sf::RenderTexture& pRender)
{
	sf::Vector2u window_size = static_cast<sf::Vector2u>(static_cast<sf::Vector2f>(ImGui::GetWindowContentRegionMax()))
		- sf::Vector2u(ImGui::GetCursorPos()) * (unsigned int)2;
	if (window_size != pRender.getSize())
	{
		pRender.create(window_size.x, window_size.y);
		return true;
	}
	return false;
}

WGE_imgui_editor::WGE_imgui_editor()
{
	mGame_render_target.create(400, 400);
	mGame_renderer.set_target_render(mGame_render_target);
	mGame.set_renderer(mGame_renderer);

	mTilemap_renderer.set_target_render(mTilemap_render_target);
	mTilemap_display.set_renderer(mTilemap_renderer);

	mTile_size = 32;

	mSelected_tile = 1;
	mTile_rotation = 0;
	mTilemap_current_snapping = snapping_full;

	mSettings.load("./editor/settings.xml");

	mTilemap_display.set_parent(mTilemap_center_node);
	mTilemap_scale = 0;

	mIs_scene_modified = false;

	mShow_debug_info = true;

	mShow_grid = true;
}

void WGE_imgui_editor::run()
{
	engine::display_window window("WGE Editor New and improved", { 640, 480 });
	ImGui::SFML::Init(window.get_sfml_window());

	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::StyleColorsDark(&style);
	style.FrameRounding = 2;
	style.FramePadding = ImVec2(2, 2);

	mGame.load("./data");
	mGame_renderer.refresh();
	mScene_list = mGame.compile_scene_list();

	sf::Clock delta_clock;
	while (window.is_open())
	{
		if (!window.poll_events())
			break;
		window.push_events_to_imgui();

		if (mIs_game_view_window_focused)
			mGame_renderer.update_events(window);

		mGame.tick();
		
		ImGui::SFML::Update(window.get_sfml_window(), delta_clock.restart());

		handle_scene_change();
		handle_undo_redo();

		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("Game"))
		{
			ImGui::MenuItem("New");
			ImGui::MenuItem("Open");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Editors"))
		{
			if (ImGui::BeginMenu("Tilemap"))
			{
				ImGui::MenuItem("Show");
				ImGui::Separator();
				ImGui::MenuItem("Tilemap Display");
				ImGui::MenuItem("Tile Settings");
				ImGui::MenuItem("Tilemap Layers");
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Arrange"))
			{
				// TODO: Collapse all windows and arrange them neat
				//       (don't know exactly how, yet..)
			}
			ImGui::MenuItem("Do Thing");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();

		draw_scene_window();
		draw_game_window();
		draw_game_view_window();
		draw_tilemap_editor_window();
		draw_log_window();
		draw_collision_settings_window();

		if (ImGui::Begin("Collision Editor"))
		{
			ImGui::Text("TODO: Use ImGui::Image to display tilemap");
		}
		ImGui::End();


		window.clear();
		ImGui::SFML::Render(window.get_sfml_window());
		window.display();
	}
	ImGui::SFML::Shutdown();
}

void WGE_imgui_editor::place_tile(engine::fvector pos)
{
	if (mTilemap_manipulator.get_layer_count() > 0)
	{
		mIs_scene_modified = true;
		rpg::tile tile;
		tile.set_position(pos);
		tile.set_atlas(mCurrent_tile_atlas->get_name(), mTilemap_manipulator.get_layer(mCurrent_layer).get_pool());
		tile.set_rotation((unsigned int)mTile_rotation);
		mCommand_manager.execute<command_set_tiles>(mCurrent_layer, mTilemap_manipulator, tile);
		mTilemap_manipulator.get_layer(mCurrent_layer).sort();
		mTilemap_display.update(mTilemap_manipulator);
	}
}

void WGE_imgui_editor::remove_tile(engine::fvector pos)
{
	if (mTilemap_manipulator.get_layer_count() > 0
		&& mTilemap_manipulator.get_layer(mCurrent_layer).find_tile(pos)) // Check if a tile will actually be deleted
	{
		mIs_scene_modified = true;
		mCommand_manager.execute<command_remove_tiles>(mCurrent_layer, mTilemap_manipulator, pos);
		mTilemap_manipulator.get_layer(mCurrent_layer).sort();
		mTilemap_display.update(mTilemap_manipulator);
	}
}

void WGE_imgui_editor::prepare_scene(engine::fs::path pPath, const std::string& pName)
{
	// Load the scene settings
	if (mScene_loader.load(pPath, pName))
	{
		// Load up the new tilemap
		mTilemap_texture = mGame.get_resource_manager().get_resource<engine::texture>("texture"
			, mScene_loader.get_tilemap_texture());
		mTilemap_manipulator.load_tilemap_xml(mScene_loader.get_tilemap());
		mTilemap_display.set_texture(mTilemap_texture);
		mTilemap_display.set_unit(static_cast<float>(mTile_size));
		mTilemap_display.update(mTilemap_manipulator);
		mCurrent_layer = 0;

		center_tilemap();

		mIs_scene_modified = false;
		mCommand_manager.clear();
	}
}

void WGE_imgui_editor::save_scene()
{
	logger::info("Saving scene");
	mTilemap_manipulator.condense_map();
	mScene_loader.set_tilemap(mTilemap_manipulator);
	mTilemap_manipulator.explode_all();
	if (!mScene_loader.save())
	{
		logger::error("Failed saving scene xml file");
		return;
	}
	mIs_scene_modified = false;
	logger::info("Scene saved");
}

void WGE_imgui_editor::draw_scene_window()
{
	if (ImGui::Begin("Scene"))
	{

		ImGui::BeginGroup();
		ImGui::Text("Scene List");
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			mScene_list = mGame.compile_scene_list();
		ImGui::QuickTooltip("Repopulate the scene list");

		ImGui::BeginChild("Scenelist", ImVec2(200, -25), true);
		for (auto& i : mScene_list)
			if (ImGui::Selectable(i.c_str(), i == mGame.get_scene().get_path()))
				mGame.get_scene().load_scene(i);
		ImGui::EndChild();

		const std::size_t scene_name_length = sizeof(mNew_scene_name_buf) / sizeof(mNew_scene_texture_name[0]);

		if (ImGui::Button("New###NewScene"))
		{
			std::memset(mNew_scene_name_buf, 0, scene_name_length);
			mNew_scene_texture_name.clear();
			ImGui::OpenPopup("Create scene");
		}
		ImGui::QuickTooltip("Create a new scene for this game");

		if (ImGui::BeginPopupModal("Create scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{

			ImGui::InputText("Name", mNew_scene_name_buf, scene_name_length);

			ImGui::TextureSelectCombo("Tilemap Texture", mGame.get_resource_manager(), &mNew_scene_texture_name);

			if (ImGui::Button("Create", ImVec2(100, 25)) && strnlen(mNew_scene_name_buf, scene_name_length) != 0)
			{
				if (mGame.create_scene(mNew_scene_name_buf, mNew_scene_texture_name))
				{
					logger::info("Scene \"" + std::string(mNew_scene_name_buf) + "\" created");
					mGame.get_scene().load_scene(mNew_scene_name_buf);
				}
				else
					logger::error("Failed to create scene \"" + std::string(mNew_scene_name_buf) + "\"");
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100, 25)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::EndGroup();

		//ImGui::SameLine();
		//ImGui::VSplitter("vsplitter", 8, &w);

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::PushItemWidth(-100);
		if (ImGui::Button("Restart"))
		{
			// This does not affect the editors
			mGame.get_scene().reload_scene();
		}
		ImGui::QuickTooltip("Restarts the current scene. This will not affect the editors.");

		ImGui::SameLine();
		if (ImGui::Button("Save"))
			save_scene();

		ImGui::Text(("Name: " + mGame.get_scene().get_path()).c_str());

		bool has_boundary = mScene_loader.has_boundary();
		if (ImGui::Checkbox("Has boundary", &has_boundary))
		{
			mIs_scene_modified = true;
			mScene_loader.set_has_boundary(has_boundary);
		}
		ImGui::QuickTooltip("The boundary is the region in which the camera will be contained. The camera cannot move out of the boundary.");

		engine::frect boundary = mScene_loader.get_boundary();
		if (ImGui::DragFloat4("Boundary", boundary.components))
		{
			mIs_scene_modified = true;
			mScene_loader.set_boundary(boundary);
		}
		ImGui::QuickTooltip("Sets the boundary's x, y, width, and height respectively. This is in unit tiles.");

		if (ImGui::Button("Open Script in Editor"))
		{
			std::string cmd = mSettings.generate_open_cmd(mScene_loader.get_script_path());
			if (std::system(("START " + cmd).c_str())) // May not be very portable
				logger::error("Failed to launch editor");
		}
		ImGui::PopItemWidth();
		ImGui::EndGroup();
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_game_window()
{
	if (ImGui::Begin("Game"))
	{

		if (ImGui::Button("Restart game", ImVec2(-0, 0)))
		{
			mGame.restart_game();
		}

		if (ImGui::Button("Open game", ImVec2(-0, 0)))
		{
			ImGui::OpenPopup("Open Game");
		}
		static engine::fs::path game_path;
		if (ImGui::FileOpenerPopup("Open Game", &game_path, true, true))
		{
			logger::info("You opened a file '" + game_path.string() + "'");
		}

		ImGui::PushItemWidth(-100);

		static char game_name_buffer[256]; // Temp
		ImGui::InputText("Name", &game_name_buffer[0], 256);
		ImGui::QuickTooltip("Name of this game.\nThis is displayed in the window title.");

		ImGui::InputInt("Tile Size", &mTile_size, 1, 2);
		ImGui::QuickTooltip("Represents both the width and height of the tiles.");

		static int target_size[2] = { 384 , 320 }; // Temp
		ImGui::DragInt2("Target Size", target_size);
		ImGui::PopItemWidth();

		if (ImGui::CollapsingHeader("Flags"))
		{
			// TODO: Add Filtering
			ImGui::BeginChild("Flag List", ImVec2(0, 100));
			for (size_t i = 0; i < 5; i++)
			{
				ImGui::Selectable(("Flag " + std::to_string(i)).c_str());
				if (ImGui::BeginPopupContextItem(("flagcxt" + std::to_string(i)).c_str()))
				{
					ImGui::MenuItem("Unset");
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
		if (ImGui::CollapsingHeader("Resource Manager"))
		{
			ImGui::BeginChild("Resource List", ImVec2(0, 200));

			ImGui::Columns(3);

			ImGui::SetColumnWidth(0, 100);
			ImGui::Text("Status");
			ImGui::NextColumn();

			ImGui::Text("Type");
			ImGui::NextColumn();

			ImGui::Text("Name");
			ImGui::NextColumn();
			ImGui::Separator();

			for (auto& i : mGame.get_resource_manager().get_resources())
			{
				ImGui::Selectable(i->is_loaded() ? "Loaded" : "Not Loaded", false, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::NextColumn();

				ImGui::Text(i->get_type().c_str());
				ImGui::NextColumn();

				ImGui::Text(i->get_name().c_str());
				ImGui::NextColumn();
			}
			ImGui::EndChild();
			ImGui::Columns(1);

			if (ImGui::Button("Reload All"))
			{
				mGame.get_resource_manager().reload_all();
			}
		}
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_game_view_window()
{
	if (ImGui::Begin("Game View"))
	{
		// This game will not recieve events if this is false
		mIs_game_view_window_focused = ImGui::IsWindowFocused();

		if (resize_to_window(mGame_render_target))
			mGame_renderer.refresh(); // refresh the engines view

		// Render the game
		mGame_renderer.draw();
		mGame_render_target.display();

		// Display on imgui window. 
		ImGui::AddBackgroundImage(mGame_render_target);
		
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(sf::Keyboard::F1))
			mShow_debug_info = !mShow_debug_info;

		if (mShow_debug_info)
		{
			const engine::fvector window_mouse_position = static_cast<engine::fvector>(ImGui::GetMousePos()) - static_cast<engine::fvector>(ImGui::GetCursorScreenPos());
			const engine::fvector view_mouse_position = mGame_renderer.window_to_game_coords(engine::vector_cast<int>(window_mouse_position));
			const engine::fvector view_tile_mouse_position = view_mouse_position / mTile_size;
			const engine::fvector tile_mouse_position = mGame_renderer.window_to_game_coords(engine::vector_cast<int>(window_mouse_position), mGame.get_scene().get_world_node());

			ImDrawList * dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(ImGui::GetCursorScreenPos(), static_cast<engine::fvector>(ImGui::GetCursorScreenPos()) + engine::fvector(300, ImGui::GetTextLineHeightWithSpacing() * 6), engine::color(0, 0, 0, 150).to_uint32(), 5);

			ImGui::BeginGroup();
			ImGui::Text("FPS: %.2f", mGame_renderer.get_fps());
			ImGui::Text("Frame: %.2fms", mGame_renderer.get_delta() * 1000);
			ImGui::Text("Mouse position:");
			ImGui::Text("        (View)  %.2f, %.2f pixels", view_mouse_position.x, view_mouse_position.y);
			ImGui::Text("                %.2f, %.2f tiles", view_tile_mouse_position.x, view_tile_mouse_position.y);
			ImGui::Text("       (World)  %.2f, %.2f tiles", tile_mouse_position.x, tile_mouse_position.y);
			ImGui::EndGroup();
			ImGui::QuickTooltip("Basic debug info.\nUse F1 to toggle.");
		}
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_tilemap_editor_window()
{
	if (ImGui::Begin("Tilemap Editor"))
	{
		ImGui::BeginChild("tilemapeditorwindow", ImVec2(-300, -1));
		if (resize_to_window(mTilemap_render_target))
		{
			engine::fvector new_size = engine::vector_cast<float, unsigned int>(mTilemap_render_target.getSize());
			mTilemap_renderer.set_target_size(new_size);
			mTilemap_renderer.refresh(); // refresh the engines view
			mTilemap_center_node.set_position(new_size / (static_cast<float>(mTile_size) * 2.f)); // Center the center node
		}

		// Render the tilemap
		mTilemap_renderer.draw();

		engine::ivector window_mouse_position = static_cast<engine::ivector>(ImGui::GetMousePos()) - static_cast<engine::ivector>(ImGui::GetCursorScreenPos());
		engine::fvector mouse_position = snap(mTilemap_renderer.window_to_game_coords(window_mouse_position, mTilemap_display), calc_snapping(mTilemap_current_snapping, mTile_size));

		// Draw previewed tile
		if (mTilemap_texture && mCurrent_tile_atlas)
		{
			mPrimitives.push_node(mTilemap_display);

			// Highlight the tile that may be replaced
			if (auto tile = mTilemap_manipulator.get_layer(mCurrent_layer).find_tile(mouse_position))
			{
				auto atlas = mTilemap_texture->get_entry(tile->get_atlas());
				engine::fvector size = atlas->get_root_frame().get_size();
				mPrimitives.add_rectangle({ tile->get_position()*mTile_size, tile->get_rotation() % 2 ? size.flip() : size }
				, { 1, 0.5f, 0.5f, 0.5f }, { 1, 0, 0, 0.7f });
			}

			mPrimitives.add_quad_texture(mTilemap_texture, mouse_position*mTile_size
				, mCurrent_tile_atlas->get_root_frame(), { 1, 1, 1, 0.7f }, mTile_rotation);


			mPrimitives.pop_node();
		}
		
		// Draw grid
		if (mShow_grid)
		{
			engine::fvector offset = mTilemap_display.get_exact_position();
			engine::fvector scale = mTilemap_display.get_absolute_scale()*static_cast<float>(mTile_size);
			engine::fvector display_size = engine::vector_cast<float, unsigned int>(mTilemap_render_target.getSize());
			draw_grid(mPrimitives, offset, scale, display_size, { 1, 1, 1, 0.7f });
			draw_grid(mPrimitives, offset + (engine::fvector(mTile_size, mTile_size) / 2) * scale
				, scale, display_size, { 1, 1, 1, 0.5f });
		}


		mPrimitives.draw_and_clear(mTilemap_renderer);

		mTilemap_render_target.display();

		// Display on window. 
		ImGui::AddBackgroundImage(mTilemap_render_target);

		ImGui::InvisibleButton("tilemaphandler", ImVec2(-1, -1));

		// Place tile
		if (mTilemap_texture && mCurrent_tile_atlas)
		{
			if (ImGui::IsItemClicked())
			{
				place_tile(mouse_position);
			}
			if (ImGui::IsItemClicked(1))
			{
				remove_tile(mouse_position);
			}
		}

		// Handle mouse interaction with tilemap window
		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(2)) // Middle mouse button is held to pan
			{
				mTilemap_display.set_position(mTilemap_display.get_position()
					+ (engine::fvector(ImGui::GetIO().MouseDelta) / static_cast<float>(mTile_size)) // Delta has to be scaled to ingame coords
					/ mTilemap_display.get_absolute_scale()); // Then scaled again to fit the zoom
			}

			if (ImGui::GetIO().MouseWheel != 0) // Middle mouse wheel zooms
			{
				mTilemap_scale += ImGui::GetIO().MouseWheel;
				mTilemap_scale = util::clamp<float>(mTilemap_scale, -2, 5);
				mTilemap_center_node.set_scale(engine::fvector(1, 1)*std::pow(2.f, mTilemap_scale));
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginGroup();

		if (ImGui::TreeNodeEx("Visual", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(-100);
			const char* snapping_items[] = { "None", "Pixel", "Eighth", "Quarter", "Full" };
			ImGui::Combo("Snapping", &mTilemap_current_snapping, snapping_items, 5);

			ImGui::Checkbox("grid", &mShow_grid);

			if (ImGui::Button("Center View"))
				center_tilemap();
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Tile", ImGuiTreeNodeFlags_DefaultOpen))
		{
			draw_tile_group();
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Layers", ImGuiTreeNodeFlags_DefaultOpen))
		{
			draw_tilemap_layers_group();
			ImGui::TreePop();
		}
		ImGui::EndGroup();
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_tile_group()
{
	ImGui::BeginChild("Tilesettingsgroup", ImVec2(0, -300));

	ImGui::BeginChild("Tile Preview", ImVec2(100, 100), true);
	if (mTilemap_texture && mCurrent_tile_atlas)
	{
		// Scale the preview image to fit the window while maintaining aspect ratio
		engine::fvector preview_size = engine::fvector(100, 100)
			- engine::fvector(ImGui::GetStyle().WindowPadding) * 2;
		engine::fvector size = mCurrent_tile_atlas->get_root_frame().get_size();
		engine::fvector scaled_size =
		{
			std::min(size.x*(preview_size.y / size.y), preview_size.x),
			std::min(size.y*(preview_size.x / size.x), preview_size.y)
		};
		ImGui::SetCursorPos(preview_size / 2 - scaled_size / 2 + ImGui::GetStyle().WindowPadding); // Center it
		ImGui::Image(mTilemap_texture->get_sfml_texture(), scaled_size, mCurrent_tile_atlas->get_root_frame()); // Draw it
	}
	else
		ImGui::TextUnformatted("No preview");
	ImGui::EndChild();
	ImGui::QuickTooltip("Preview of tile to place.");
	//TODO: Add option to change background color

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::PushItemWidth(-100);

	ImGui::DragInt("Rotation", &mTile_rotation, 0.2f, 0, 3, u8"%.0f x90\u00B0");

	ImGui::PopItemWidth();
	ImGui::EndGroup();

	ImGui::BeginChild("Tile List", ImVec2(0, 0), true);
	if (mTilemap_texture)
	{
		for (auto i : mTilemap_texture->get_texture_atlas().get_all())
		{
			// IDEA: Possibly add a preview for each tile in the list
			if (ImGui::Selectable(i->get_name().c_str(), mCurrent_tile_atlas == i))
				mCurrent_tile_atlas = i;
		}
	}
	else
		ImGui::TextUnformatted("No texture");
	ImGui::EndChild();
	ImGui::EndChild();
}

void WGE_imgui_editor::draw_tilemap_layers_group()
{
	ImGui::BeginGroup();
	ImGui::BeginChild("Layer List", ImVec2(0, -25), true);
	static bool layer_visible = false;
	ImGui::Columns(2, 0, false);
	ImGui::SetColumnWidth(0, 25);
	const size_t layer_count = mTilemap_manipulator.get_layer_count();
	for (size_t i = 0; i < layer_count; i++)
	{
		size_t layer_index = layer_count - i - 1; // Reverse so the top layer is at the top of the list
		rpg::tilemap_layer& layer = mTilemap_manipulator.get_layer(layer_index);

		// Checkbox on left that enables or disables a layer
		bool is_visible = mTilemap_display.is_layer_visible(layer_index);
		if (ImGui::Checkbox(ImGui::IdOnly("Tilemaplayer" + std::to_string(layer_index)).c_str(), &is_visible))
			mTilemap_display.set_layer_visible(layer_index, is_visible);
		ImGui::NextColumn();

		if (ImGui::Selectable(ImGui::NameId(layer.get_name(), "layer" + std::to_string(layer_index)).c_str(), mCurrent_layer == layer_index))
		{
			mCurrent_layer = layer_index;
			if (ImGui::IsMouseDoubleClicked(0))
				ImGui::OpenPopup("Rename Layer");
		}
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::Separator();

	if (ImGui::Button("New"))
	{
		mCurrent_layer = mTilemap_manipulator.insert_layer(mCurrent_layer);
		mTilemap_display.update(mTilemap_manipulator);
	}

	ImGui::SameLine();
	if (ImGui::Button("Rename"))
		ImGui::OpenPopup("Rename Layer");
	if (ImGui::BeginPopupModal("Rename Layer", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char name[256]; // TODO: default to original name
		bool return_pressed = ImGui::InputText("Name", name, 256, ImGuiInputTextFlags_EnterReturnsTrue);
		bool renamebt_pressed = ImGui::Button("Rename", ImVec2(100, 25));
		if ((return_pressed || renamebt_pressed) && strnlen(name, sizeof(name)/sizeof(name[0])) != 0)
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).set_name(name);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete") && mTilemap_manipulator.get_layer_count() > 0)
		ImGui::OpenPopup("Confirm layer removal");

	if (mTilemap_manipulator.get_layer_count() > 0
		&& ImGui::ConfirmPopup("Confirm layer removal"
		, ("Are you sure you want to remove layer \""
			+ mTilemap_manipulator.get_layer(mCurrent_layer).get_name() + "\"?").c_str()) == 1)
	{
		mTilemap_manipulator.remove_layer(mCurrent_layer);
		mCurrent_layer = std::min(mCurrent_layer, mTilemap_manipulator.get_layer_count() - 1);
		mTilemap_display.update(mTilemap_manipulator);
	}

	ImGui::SameLine();
	if (ImGui::ArrowButton("Move Up", ImGuiDir_Up) && mCurrent_layer != mTilemap_manipulator.get_layer_count() - 1)
	{
		mTilemap_manipulator.move_layer(mCurrent_layer, mCurrent_layer + 1);
		++mCurrent_layer;
		mTilemap_display.update(mTilemap_manipulator);
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Move Down", ImGuiDir_Down) && mCurrent_layer != 0)
	{
		mTilemap_manipulator.move_layer(mCurrent_layer, mCurrent_layer - 1);
		--mCurrent_layer;
		mTilemap_display.update(mTilemap_manipulator);
	}
	ImGui::EndGroup();
}

void WGE_imgui_editor::center_tilemap()
{
	mTilemap_display.set_position(-mTilemap_manipulator.get_center_point());
}

void WGE_imgui_editor::draw_collision_settings_window()
{
	if (ImGui::Begin("Collision Settings"))
	{
		ImGui::PushItemWidth(-100);
		static int current_snapping = 0; // Temp
		const char* snapping_items[] = { "None", "Pixel", "Eighth", "Quarter", "Full" };
		ImGui::Combo("Snapping", &current_snapping, snapping_items, 5);

		if (ImGui::CollapsingHeader("Collision Box"))
		{
			static int current_type = 0; // Temp
			const char* coll_type_items[] = { "Wall", "Trigger", "Button", "Door" };
			ImGui::Combo("Type", &current_type, coll_type_items, 4);

			static char group_buf[256]; // Temp
			ImGui::InputText("Group", group_buf, 256);
			ImGui::QuickTooltip("Group to assosiate this collision box with.\nThis is used in scripts to enable/disable boxes or calling functions when collided.");

			static float coll_rect[4] = { 0, 0, 1, 1 }; // Temp
			ImGui::DragFloat4("Rect", coll_rect, 1);
		}

		if (ImGui::CollapsingHeader("Door"))
		{
			static char door_name_buf[256]; // Temp
			ImGui::InputText("Name", door_name_buf, 256);

			static float coll_rect[2] = { 0, 0 }; // Temp
			ImGui::DragFloat2("Offset", coll_rect, 0.2f);
			ImGui::QuickTooltip("This will offset the player when they come through this door.\nUsed to prevent player from colliding with the door again and going back.");

			// TODO: Use a combo instead
			static char door_dest_scene_buf[256]; // Temp
			ImGui::InputText("Dest. Scene", door_dest_scene_buf, 256);
			ImGui::QuickTooltip("The scene to load when the player enters this door.");

			// TODO: Use a combo instead
			static char door_dest_door_buf[256]; // Temp
			ImGui::InputText("Dest. Door", door_dest_door_buf, 256);
			ImGui::QuickTooltip("The destination door in the new loaded scene.");

		}
		ImGui::PopItemWidth();
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_log_window()
{
	if (ImGui::Begin("Log"))
	{
		const auto& log = logger::get_log();

		// Help keep track of changes in the log
		static size_t last_log_size = 0;

		// If the window is scrolled to the bottom, maintain it at the bottom
		// To prevent it from locking the users mousewheel input, it will only lock the scroll
		// when the log actually changes.
		bool lock_scroll_at_bottom = ImGui::GetScrollY() == ImGui::GetScrollMaxY() && last_log_size != log.size();

		ImGui::Columns(2, 0, false);
		ImGui::SetColumnWidth(0, 60);
		const size_t start = log.size() >= 256 ? log.size() - 256 : 0; // Limit to 256
		for (size_t i = start; i < log.size(); i++)
		{
			switch (log[i].type)
			{
			case logger::level::info:
				ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 }); // White
				ImGui::TextUnformatted("Info");
				break;
			case logger::level::debug:
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 1, 1, 1 }); // Cyan-ish
				ImGui::TextUnformatted("Debug");
				break;
			case logger::level::warning:
				ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 0.5f, 1 }); // Yellow-ish
				ImGui::TextUnformatted("Warning");
				break;
			case logger::level::error:
				ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.5f, 0.5f, 1 }); // Red
				ImGui::TextUnformatted("Error");
				break;
			}
			ImGui::NextColumn();

			// The actual message. Has the same color as the message type.
			ImGui::TextUnformatted(log[i].msg.c_str());
			ImGui::PopStyleColor();

			if (log[i].is_file)
			{
				std::string file_info = log[i].file;
				if (log[i].row >= 0) // Line info
				{
					file_info += " (" + std::to_string(log[i].row);
					if (log[i].column >= 0) // Column info
						file_info += ", " + std::to_string(log[i].column);
					file_info += ")";
				}

				// Filepath. Gray.
				ImGui::TextColored({ 0.7f, 0.7f, 0.7f, 1 }, file_info.c_str());

				ImGui::SameLine();
				if (ImGui::ArrowButton(("logfileopen" + std::to_string(i)).c_str(), ImGuiDir_Right))
				{
					std::string cmd = mSettings.generate_open_cmd(log[i].file);
					std::system(("START " + cmd).c_str()); // May not be very portable
				}
				ImGui::QuickTooltip("Open file in editor.");
			}
			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		if (lock_scroll_at_bottom)
			ImGui::SetScrollHere();
		last_log_size = log.size();
	}
	ImGui::End();
}

void WGE_imgui_editor::handle_undo_redo()
{
	if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		return;
	// Undo
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(sf::Keyboard::Z))
	{
		if (mCommand_manager.undo())
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).sort();
			mTilemap_display.update(mTilemap_manipulator);
		}
	}

	// Redo
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(sf::Keyboard::Y))
	{
		if (mCommand_manager.redo())
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).sort();
			mTilemap_display.update(mTilemap_manipulator);
		}
	}
}

void WGE_imgui_editor::handle_scene_change()
{		// Has the current scene changed?
	if (mScene_loader.get_name() != mGame.get_scene().get_name() && !ImGui::IsPopupOpen("###askforsave"))
	{
		if (mIs_scene_modified)
			ImGui::OpenPopup("###askforsave");
		else
			prepare_scene(mGame.get_source_path() / "scenes", mGame.get_scene().get_name());
	}

	int scene_save_answer = ImGui::ConfirmPopup("Save?###askforsave", "Do you want to save this scene before moving on?");
	if (scene_save_answer == 1) // yes
	{
		save_scene();
		prepare_scene(mGame.get_source_path() / "scenes", mGame.get_scene().get_name());
	}
	else if (scene_save_answer == 2) // no
	{
		// No save
		prepare_scene(mGame.get_source_path() / "scenes", mGame.get_scene().get_name());
	}
}

engine::fvector WGE_imgui_editor::calc_snapping(int pSnapping, int pTile_size)
{
	switch (pSnapping)
	{
	default:
	case snapping_none:    return { 0, 0 };
	case snapping_pixel:   return engine::fvector(1, 1) / pTile_size;
	case snapping_eight:   return { 0.25f, 0.25f };
	case snapping_quarter: return { 0.5f, 0.5f };
	case snapping_full:    return { 1, 1 };
	}
}
