#include <rpg/editor.hpp>

using namespace editors;

#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>

#include <engine/logger.hpp>
#include <engine/utility.hpp>

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


bool command_manager::execute(std::shared_ptr<command> pCommand)
{
	assert(!mCurrent);
	mRedo.clear();
	mUndo.push_back(pCommand);
	return pCommand->execute();
}

bool command_manager::add(std::shared_ptr<command> pCommand)
{
	assert(!mCurrent);
	mRedo.clear();
	mUndo.push_back(pCommand);
	return true;
}

void command_manager::start(std::shared_ptr<command> pCommand)
{
	mCurrent = pCommand;
}

void command_manager::complete()
{
	assert(mCurrent);
	mRedo.clear();
	mUndo.push_back(mCurrent);
	mCurrent.reset();
}

bool command_manager::undo()
{
	if (mUndo.size() == 0)
		return false;
	auto undo_cmd = mUndo.back();
	mUndo.pop_back();
	mRedo.push_back(undo_cmd);
	return undo_cmd->undo();
}

bool command_manager::redo()
{
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
	command_set_tiles(int pLayer, rpg::tilemap_manipulator* pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = pTilemap_manipulator;
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
		{
			mTilemap_manipulator->get_layer(mLayer).remove_tile(i.get_position());
		}

		// Place all replaced tiles back
		for (auto& i : mReplaced_tiles)
		{
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);
		}

		return true;
	}

	void add(rpg::tile& pTile)
	{
		mTiles.push_back(pTile);
	}

private:
	int mLayer;

	std::vector<rpg::tile> mReplaced_tiles;
	std::vector<rpg::tile> mTiles;

	rpg::tilemap_manipulator* mTilemap_manipulator;
};

class command_remove_tiles :
	public command
{
public:
	command_remove_tiles(int pLayer, rpg::tilemap_manipulator* pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = pTilemap_manipulator;
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
	int mLayer;

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
// tilemap_editor
// ##########

tilemap_editor::tilemap_editor()
{
	mPreview.set_color({ 1, 1, 1, 0.78f });

	mCurrent_tile = 0;
	mRotation = 0;
	mLayer = 0;

	mIs_highlight = false;

	mState = state::none;
}

bool tilemap_editor::open_editor()
{
	clean();

	// Get tilemap texture
	// The user should be allowed to add a texture when there isn't any.
	mTexture = mTilemap_display.get_texture();
	if (mTexture)
	{
		mCurrent_texture_name = mLoader.get_tilemap_texture();
		mTile_list = std::move(mTexture->compile_list());
		mPreview.set_texture(mTexture);
	}
	update_preview();
	return true;
}

int tilemap_editor::draw(engine::renderer & pR)
{
	// Editing is not allowed as there are no tiles to use.
	if (mTile_list.empty())
		return 1;
	update_zoom(pR);

	const engine::fvector mouse_position = pR.get_mouse_position(*this);

	const engine::fvector tile_position_exact = mouse_position / get_unit();
	const engine::fvector tile_position
		= false // Full tile
		? engine::fvector(tile_position_exact * 2).floor() / 2
		: engine::fvector(tile_position_exact).floor();

	switch (mState)
	{
	case state::none:
	{
		if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_left)
			&& pR.is_key_down(engine::renderer::key_code::LShift))
		{
			mState = state::drawing_region;
			mLast_tile = tile_position;
		}
		else if (pR.is_key_down(engine::renderer::key_code::LControl))
		{
			if (pR.is_key_pressed(engine::renderer::key_code::Z)) // Undo
			{
				mCommand_manager.undo();
				update_tilemap();
			}
			else if (pR.is_key_pressed(engine::renderer::key_code::Y)) // Redo
			{
				mCommand_manager.redo();
				update_tilemap();
			}
		}
		else if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_left))
		{
			mState = state::drawing;
			draw_tile_at(tile_position); // Draw first tile
			mLast_tile = tile_position;
		}
		else if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_right))
		{
			mState = state::erasing;
			erase_tile_at(tile_position); // Erase first tile
			mLast_tile = tile_position;
		}
		else if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_middle))
		{
			copy_tile_type_at(tile_position); // Copy tile (no need for a specific state for this)
		}
		else if (pR.is_key_pressed(engine::renderer::key_code::Period))
		{
			next_tile();
		}
		else if (pR.is_key_pressed(engine::renderer::key_code::Comma))
		{
			previous_tile();
		}
		else if (pR.is_key_pressed(engine::renderer::key_code::R))
		{
			rotate_clockwise();
		}
		break;
	}
	case state::drawing:
	{
		if (!pR.is_mouse_down(engine::renderer::mouse_button::mouse_left))
		{
			mState = state::none;
			break;
		}

		// Don't draw at the same place twice
		if (mLast_tile == tile_position)
			break;
		mLast_tile = tile_position;

		draw_tile_at(tile_position);

		break;
	}
	case state::erasing:
	{
		if (!pR.is_mouse_down(engine::renderer::mouse_button::mouse_right))
		{
			mState = state::none;
			break;
		}

		// Don't erase at the same place twice
		if (mLast_tile == tile_position)
			break;
		mLast_tile = tile_position;

		erase_tile_at(tile_position);

		break;
	}
	case state::drawing_region:
	{
		// Apply the region only after releasing left mouse button
		if (!pR.is_mouse_down(engine::renderer::mouse_button::mouse_left))
		{
			mState = state::none;

			// TODO : Place tiles
		}

		break;
	}
	}

	tick_highlight(pR);

	mTilemap_display.draw(pR);

	mGrid.set_major_size({ get_unit(), get_unit() });
	mGrid.update_grid(pR);
	mGrid.draw(pR);
	if (mState == state::drawing_region)
	{
		// TODO : Draw rectangle specifying region
	}
	else
	{
		mPreview.set_position(tile_position);
		mPreview.draw(pR);
	}

	return 0;
}

void tilemap_editor::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mTilemap_group = std::make_shared<engine::terminal_command_group>();
	mTilemap_group->set_root_command("tilemap");

	mTilemap_group->add_command("clear",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mTilemap_manipulator.clear();
		update_tilemap();
		return true;
	}, "- Clear the entire tilemap (Warning: Can't undo)");

	/*mTilemap_group->add_command("shift",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() < 2)
		{
			logger::error("Not enough arguments");
			return false;
		}

		engine::fvector shift_amount;
		try {
			shift_amount = read_args_vector(pArgs);
		}
		catch (...)
		{
			logger::error("Invalid offset input");
			return false;
		}

		if (pArgs.size() >= 3)
		{
			if (pArgs[2].get_raw() == "current")
			{
				mTilemap_manipulator.shift(shift_amount, mLayer);
			}
			else
			{
				int layer;
				try {
					layer = util::to_numeral<int>(pArgs[2]);
				}
				catch (...)
				{
					logger::error("Invalid layer input");
					return false;
				}

				mTilemap_manipulator.shift(shift_amount, layer);
			}
		}
		else
		{
			mTilemap_manipulator.shift(shift_amount); // Shift entire tilemap
		}
		update_tilemap();
		return true;
	}, "<X> <Y> [Layer#/current] - Shift the entire/layer of tilemap (Warning: Can't undo)");*/

	mTilemap_group->set_enabled(false);
	pTerminal.add_group(mTilemap_group);
}

void tilemap_editor::copy_tile_type_at(engine::fvector pAt)
{
	rpg::tile* t = mTilemap_manipulator.get_layer(mLayer).find_tile(pAt);
	if (!t)
		return;

	for (size_t i = 0; i < mTile_list.size(); i++) // Find tile in tile_list and set it as current tile
	{
		if (mTile_list[i] == t->get_atlas())
		{
			mCurrent_tile = i;
			update_preview();
			break;
		}
	}
}

void tilemap_editor::draw_tile_at(engine::fvector pAt)
{
	assert(!mTile_list.empty());
	rpg::tile* ntile = mTilemap_manipulator.get_layer(mLayer).set_tile(pAt, mTile_list[mCurrent_tile], mRotation);

	auto command = std::make_shared<command_set_tiles>(mLayer, &mTilemap_manipulator);
	command->add(*ntile);

	mCommand_manager.execute(command);
	update_tilemap();
}

void tilemap_editor::erase_tile_at(engine::fvector pAt)
{
	auto command = std::make_shared<command_remove_tiles>(mLayer, &mTilemap_manipulator);
	command->add(pAt);

	mCommand_manager.execute(command);
	mTilemap_display.update(mTilemap_manipulator);
	update_tilemap();
}

void tilemap_editor::next_tile()
{
	++mCurrent_tile %= mTile_list.size();
	update_preview();
}

void tilemap_editor::previous_tile()
{
	mCurrent_tile = mCurrent_tile == 0 ? (mTile_list.size() - 1) : (mCurrent_tile - 1);
	update_preview();
}


void tilemap_editor::rotate_clockwise()
{
	++mRotation %= 4;
	update_preview();
}

void tilemap_editor::update_preview()
{
	if (!mTexture)
		return;
	mPreview.set_texture_rect(mTexture->get_entry(mTile_list[mCurrent_tile])->get_root_frame());

	mPreview.set_rotation(90.f * mRotation);

	// Align the preview correctly after the rotation
	// Possibly could just use engine::anchor::center
	switch (mRotation)
	{
	case 0:
		mPreview.set_anchor(engine::anchor::topleft);
		break;
	case 1:
		mPreview.set_anchor(engine::anchor::bottomleft);
		break;
	case 2:
		mPreview.set_anchor(engine::anchor::bottomright);
		break;
	case 3:
		mPreview.set_anchor(engine::anchor::topright);
		break;
	}
}

void tilemap_editor::update_highlight()
{
	if (mIs_highlight)
		mTilemap_display.highlight_layer(mLayer, { 200, 255, 200, 255 }, { 50, 50, 50, 100 });
	else
		mTilemap_display.remove_highlight();
}

void tilemap_editor::update_tilemap()
{
	mTilemap_display.update(mTilemap_manipulator);
	update_highlight();
}

void tilemap_editor::tick_highlight(engine::renderer& pR)
{
	if (pR.is_key_pressed(engine::renderer::key_code::RShift))
	{
		mIs_highlight = !mIs_highlight;
		update_highlight();
	}
}

void tilemap_editor::apply_texture()
{
	const std::string tilemap_texture_name = ""; // Texture name to load

	logger::info("Applying tilemap Texture '" + tilemap_texture_name + "'...");

	assert(mGame != nullptr);
	auto new_texture = mGame->get_resource_manager().get_resource<engine::texture>("texture", tilemap_texture_name);
	if (!new_texture)
	{
		logger::error("Failed to load texture '" + tilemap_texture_name + "'");
		return;
	}

	mTexture = new_texture;

	mTilemap_display.set_texture(mTexture);
	update_tilemap();
	mTile_list = std::move(mTexture->compile_list());
	assert(mTile_list.size() != 0);

	mCurrent_tile = 0;

	mPreview.set_texture(mTexture);

	update_preview();

	mCurrent_texture_name = tilemap_texture_name;

	logger::info("Tilemap texture applied");
}

bool tilemap_editor::save()
{
	auto& doc = mLoader.get_document();
	auto ele_map = mLoader.get_tilemap();

	logger::info("Saving tilemap...");

	// Update tilemap texture name
	auto ele_texture = ele_map->FirstChildElement("texture");
	ele_texture->SetText(mCurrent_texture_name.c_str());

	auto ele_layer = ele_map->FirstChildElement("layer");
	while (ele_layer)
	{
		ele_layer->DeleteChildren();
		doc.DeleteNode(ele_layer);
		ele_layer = ele_map->FirstChildElement("layer");
	}
	mTilemap_manipulator.condense_map();
	mTilemap_manipulator.generate(doc, ele_map);
	doc.SaveFile(mLoader.get_scene_path().c_str());

	logger::info("Tilemap saved");

	return editor::save();
}

void tilemap_editor::clean()
{
	mTile_list.clear();
	mLayer = 0;
	mRotation = 0;
	mIs_highlight = false;
	mCurrent_tile = 0;
	mTexture = nullptr;
	mCurrent_texture_name.clear();
	mPreview.set_texture(nullptr);
	mCommand_manager.clear();
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
				mCommand_manager.execute(std::make_shared<command_remove_wall>(mSelection, &mContainer));
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

	mContainer.generate_xml(mLoader.get_document(), mLoader.get_collisionboxes());
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

static void replace_all_with(std::string& pVal, const std::string& pTarget, const std::string& pNew)
{
	if (pTarget.length() > pVal.length()
		|| pTarget.empty() || pVal.empty())
		return;
	for (size_t i = 0; i < pVal.length() - pTarget.length() + 1; i++)
	{
		if (std::string(pVal.begin() + i, pVal.begin() + i + pTarget.size()) == pTarget) // Compare this range
		{
			pVal.replace(i, pTarget.length(), pNew); // Replace
			i += pNew.length() - 1; // Skip the new text
		}
	}
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
	replace_all_with(modified_param, "%filename%", pFilepath);
	return "\"" + mPath + "\" " + modified_param;
}

std::string editor_settings_loader::generate_opento_cmd(const std::string & pFilepath, size_t pRow, size_t pCol)
{
	std::string modified_param = mOpento_param;
	replace_all_with(modified_param, "%filename%", pFilepath);
	replace_all_with(modified_param, "%row%", std::to_string(pRow));
	replace_all_with(modified_param, "%col%", std::to_string(pCol));
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
	return pName + "##" + pId;
}

static inline std::string IdOnly(const std::string& pId)
{
	return "##" + pId;
}

// Shortcut for adding a text only tooltip to the last item
static inline void quickTooltip(const char * pString)
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

}



// Resizes a render texture if the imgui window was changed size.
// Works best if this is the first thing drawn in the window.
// Returns true if the texture was actually changed.
static inline bool resize_to_window(sf::RenderTexture& pRender)
{
	sf::Vector2u window_size = static_cast<sf::Vector2u>(ImGui::GetWindowContentRegionMax())
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

	mSettings.load("./editor/settings.xml");

	mTilemap_display.set_parent(mTilemap_center_node);
	mTilemap_scale = 0;
}

void WGE_imgui_editor::run()
{
	engine::display_window window("WGE Editor New and improved", { 640, 480 });
	ImGui::SFML::Init(window.get_sfml_window());

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2;
	style.FramePadding = ImVec2(2, 2);
	ImGui::StyleColorsDark(&style);

	float colors[3] = { 0, 0, 0 };

	const auto scenes = rpg::get_scene_list();

	// Scene Settings
	size_t selected_scene = 1;

	int val = 0;

	std::string data;
	data.resize(100);

	mGame.load("./data");
	mGame_renderer.refresh();


	sf::Clock delta_clock;
	while (window.is_open())
	{
		if (!window.poll_events())
			break;
		window.push_events_to_imgui();

		if (mIs_game_view_window_focused)
			mGame_renderer.update_events(window);

		mGame.tick();

		// Has the current scene changed?
		if (mScene_loader.get_name() != mGame.get_scene().get_name())
		{
			// Load the scene settings
			if (mScene_loader.load(mGame.get_source_path()/"scenes", mGame.get_scene().get_name()))
			{
				// Load up the new tilemap
				mTilemap_texture = mGame.get_resource_manager().get_resource<engine::texture>("texture"
					, mScene_loader.get_tilemap_texture());
				mTilemap_manipulator.load_tilemap_xml(mScene_loader.get_tilemap());
				mTilemap_display.set_texture(mTilemap_texture);
				mTilemap_display.set_unit(static_cast<float>(mTile_size));
				mTilemap_display.update(mTilemap_manipulator);

				// Center the tilemap view
				mTilemap_display.set_position(-mTilemap_manipulator.get_center_point());
			}
		}
		
		ImGui::SFML::Update(window.get_sfml_window(), delta_clock.restart());

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

		if (ImGui::Begin("Scene"))
		{
			static float w = 200;

			ImGui::BeginGroup();
			ImGui::Text("Scene List");
			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
			{
				mScene_list = mGame.compile_scene_list();
			}
			ImGui::BeginChild("Scenelist", ImVec2(w, 0), true);
			for (auto& i : mScene_list)
			{
				if (ImGui::Selectable(i.c_str(), i == mGame.get_scene().get_path()))
				{
					mGame.get_scene().load_scene(i);
				}
			}
			ImGui::EndChild();
			ImGui::EndGroup();

			//ImGui::SameLine();
			//ImGui::VSplitter("vsplitter", 8, &w);

			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::PushItemWidth(-100);
			if (ImGui::Button("Restart"))
			{
				mGame.get_scene().reload_scene();
			}
			ImGui::Text(("Name: " + mGame.get_scene().get_path()).c_str());

			static bool has_boundary = false; // Temp
			ImGui::Checkbox("Has boundary", &has_boundary);

			engine::frect boundary = mScene_loader.get_boundary();
			static float boundary_values[4] = {0, 0, 0, 0}; // temp
			if (ImGui::DragFloat4("Boundary", boundary_values))
			{
			}
			ImGui::Button("Open Script in Editor");
			ImGui::PopItemWidth();
			ImGui::EndGroup();
		}
		ImGui::End();

		draw_game_window();
		draw_game_view_window();

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
			mTilemap_render_target.display();

			// Display on window. 
			ImGui::AddBackgroundImage(mTilemap_render_target);

			ImGui::InvisibleButton("tilemaphandler", ImVec2(-1, -1));

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
					logger::debug("Tilemap Editor: Changed zoom to [" + std::to_string(mTilemap_scale) + "]");
					mTilemap_center_node.set_scale(engine::fvector(1, 1)*std::pow(2.f, mTilemap_scale));
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::BeginGroup();
			draw_tile_group();
			draw_tilemap_layers_group();
			ImGui::EndGroup();
		}
		ImGui::End();

		if (ImGui::Begin("Collision Editor"))
		{
			ImGui::Text("TODO: Use ImGui::Image to display tilemap");
		}
		ImGui::End();

		draw_log();
		draw_collision_settings_window();

		window.clear();
		ImGui::SFML::Render(window.get_sfml_window());
		window.display();
	}
	ImGui::SFML::Shutdown();
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
		ImGui::quickTooltip("Name of this game.\nThis is displayed in the window title.");

		ImGui::InputInt("Tile Size", &mTile_size, 1, 2);
		ImGui::quickTooltip("Represents both the width and height of the tiles.");

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

		// TODO: Add debug info (FPS, Delta, etc..)
		// ImGui::Text("this is debug");
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
			- engine::fvector(ImGui::GetStyle().WindowPadding)*2;
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
	ImGui::quickTooltip("Preview of tile to place.");
	//TODO: Add option to change background color

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::PushItemWidth(-100);

	ImGui::DragInt("Rotation", &mTile_rotation, 1.f, 0, 3, "%d x90");

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
	ImGui::TextUnformatted("Layers");
	ImGui::Separator();
	ImGui::BeginChild("Layer List", ImVec2(0, -25));
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

		std::string popup_name = ImGui::NameId("Rename layer \"" + layer.get_name() + "\"", std::to_string(layer_index));
		if (ImGui::Selectable(layer.get_name().c_str(), false))
			ImGui::OpenPopup(popup_name.c_str());

		if (ImGui::BeginPopup(popup_name.c_str()))
		{
			ImGui::TextUnformatted("TODO: Implement layer renaming");
			ImGui::EndPopup();
		}
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::Separator();

	ImGui::Button("New");

	ImGui::SameLine();
	ImGui::Button("Delete");

	ImGui::SameLine();
	ImGui::ArrowButton("Move Up", ImGuiDir_Up);
	ImGui::SameLine();
	ImGui::ArrowButton("Move Down", ImGuiDir_Down);
	ImGui::EndGroup();
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
			ImGui::quickTooltip("Group to assosiate this collision box with.\nThis is used in scripts to enable/disable boxes or calling functions when collided.");

			static float coll_rect[4] = { 0, 0, 1, 1 }; // Temp
			ImGui::DragFloat4("Rect", coll_rect, 1);
		}

		if (ImGui::CollapsingHeader("Door"))
		{
			static char door_name_buf[256]; // Temp
			ImGui::InputText("Name", door_name_buf, 256);

			static float coll_rect[2] = { 0, 0 }; // Temp
			ImGui::DragFloat2("Offset", coll_rect, 0.2f);
			ImGui::quickTooltip("This will offset the player when they come through this door.\nUsed to prevent player from colliding with the door again and going back.");

			// TODO: Use a combo instead
			static char door_dest_scene_buf[256]; // Temp
			ImGui::InputText("Dest. Scene", door_dest_scene_buf, 256);
			ImGui::quickTooltip("The scene to load when the player enters this door.");

			// TODO: Use a combo instead
			static char door_dest_door_buf[256]; // Temp
			ImGui::InputText("Dest. Door", door_dest_door_buf, 256);
			ImGui::quickTooltip("The destination door in the new loaded scene.");

		}
		ImGui::PopItemWidth();
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_log()
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
				ImGui::quickTooltip("Open file in editor.");
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
