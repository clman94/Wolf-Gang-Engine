#include <rpg/editor.hpp>

using namespace editors;

#include <vector>
#include <memory>
#include <engine/parsers.hpp>

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
	mRedo.clear();
	mUndo.push_back(pCommand);
	return pCommand->execute();
}

bool command_manager::add(std::shared_ptr<command> pCommand)
{
	mRedo.clear();
	mUndo.push_back(pCommand);
	return true;
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

void command_manager::clean()
{
	mUndo.clear();
	mRedo.clear();
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
			mTilemap_manipulator->explode_tile(i.get_position(), mLayer);
			auto replaced_tile = mTilemap_manipulator->get_tile(i.get_position(), mLayer);
			if (replaced_tile)
				mReplaced_tiles.push_back(*replaced_tile);
		}

		// Replace all the tiles
		for (auto& i : mTiles)
		{
			mTilemap_manipulator->set_tile(i, mLayer);
		}
		return true;
	}

	bool undo()
	{
		assert(mTilemap_manipulator);

		// Remove all placed tiles
		for (auto& i : mTiles)
		{
			mTilemap_manipulator->remove_tile(i.get_position(), mLayer);
		}

		// Place all replaced tiles back
		for (auto& i : mReplaced_tiles)
		{
			mTilemap_manipulator->set_tile(i, mLayer);
		}

		return true;
	}

	bool redo()
	{
		return execute();
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
			mTilemap_manipulator->explode_tile(i, mLayer);
			auto replaced_tile = mTilemap_manipulator->get_tile(i, mLayer);
			if (replaced_tile)
			{
				mRemoved_tiles.push_back(*replaced_tile);
				mTilemap_manipulator->remove_tile(i, mLayer);
			}
		}

		return true;
	}

	bool undo()
	{
		for (auto& i : mRemoved_tiles)
		{
			mTilemap_manipulator->set_tile(i, mLayer);
		}
		return true;
	}

	bool redo()
	{
		return execute();
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

void tgui_list_layout::updateWidgetPositions()
{
	auto& widgets = getWidgets();
	Widget::Ptr last;
	for (auto i : widgets)
	{
		if (last)
		{
			auto pos = last->getPosition();
			pos.y += last->getFullSize().y;
			i->setPosition(pos);
		}

		last = i;
	}
}

editor_gui::editor_gui()
{
	mLayout = std::make_shared<tgui_list_layout>();
	mLayout->setBackgroundColor({ 0, 0, 0,  90});
	mLayout->setSize(200, 1000);
	mLayout->hide();

	mLb_fps = std::make_shared<tgui::Label>();
	mLb_fps->setMaximumTextWidth(0);
	mLb_fps->setTextColor({ 200, 200, 200, 255 });
	mLb_fps->setText("FPS: N/A");
	mLayout->add(mLb_fps);

	mLb_mouse = tgui::Label::copy(mLb_fps);
	mLb_mouse->setText("(n/a, n/a)\n(n/a, n/a)");
	mLb_mouse->setTextSize(14);
	mLayout->add(mLb_mouse);

	mLb_scene = tgui::Label::copy(mLb_fps);
	mLb_scene->setText("n/a");
	mLb_scene->setTextSize(16);
	mLayout->add(mLb_scene);

	mEditor_layout = std::make_shared<tgui_list_layout>();
	mEditor_layout->setSize(mLayout->getSize().x, 1000);
	mEditor_layout->setBackgroundColor({ 0, 0, 0, 0 });
	mLayout->add(mEditor_layout);
}

void editor_gui::clear()
{
	mEditor_layout->removeAllWidgets();
}

tgui::Label::Ptr editor_gui::add_label(const std::string & pText, tgui::Container::Ptr pContainer)
{
	auto nlb = tgui::Label::copy(mLb_fps);
	nlb->setText(pText);
	nlb->setTextStyle(sf::Text::Bold);
	if (pContainer)
		pContainer->add(nlb);
	else
		mEditor_layout->add(nlb);
	return nlb;
}

tgui::Label::Ptr editors::editor_gui::add_small_label(const std::string & text, tgui::Container::Ptr pContainer)
{
	auto label = add_label(text, pContainer);
	label->setTextSize(10);
	return label;
}

tgui::TextBox::Ptr editor_gui::add_textbox(tgui::Container::Ptr pContainer)
{
	auto ntb = std::make_shared<tgui::TextBox>();
	ntb->setSize(sf::Vector2f(200, 25));
	if (pContainer)
		pContainer->add(ntb);
	else
		mEditor_layout->add(ntb);
	return ntb;
}

tgui::ComboBox::Ptr editors::editor_gui::add_combobox(tgui::Container::Ptr pContainer)
{
	auto ncb = std::make_shared<tgui::ComboBox>();
	ncb->setSize(sf::Vector2f(200, 25));
	if (pContainer)
		pContainer->add(ncb);
	else
		mEditor_layout->add(ncb);
	ncb->setItemsToDisplay(10);
	return ncb;
}

tgui::CheckBox::Ptr editors::editor_gui::add_checkbox(const std::string& text, tgui::Container::Ptr pContainer)
{
	auto ncb = std::make_shared<tgui::CheckBox>();
	ncb->setText(text);
	ncb->uncheck();
	if (pContainer)
		pContainer->add(ncb);
	else
		mEditor_layout->add(ncb);
	return ncb;
}

tgui::Button::Ptr editors::editor_gui::add_button(const std::string& text, tgui::Container::Ptr pContainer)
{
	auto nbt = std::make_shared<tgui::Button>();
	nbt->setText(text);
	if (pContainer)
		pContainer->add(nbt);
	else
		mEditor_layout->add(nbt);
	return nbt;
}

std::shared_ptr<tgui_list_layout> editors::editor_gui::add_sub_container(tgui::Container::Ptr pContainer)
{
	auto slo = std::make_shared<tgui_list_layout>();
	slo->setBackgroundColor({ 0, 0, 0,  0 });
	slo->setSize(200, 500);
	if (pContainer)
		pContainer->add(slo);
	else
		mEditor_layout->add(slo);
	return slo;
}

void editor_gui::get_scene(std::string& pScene_name)
{
    mScene_name = pScene_name;
}

void editor_gui::refresh_renderer(engine::renderer & pR)
{
	pR.get_tgui().add(mLayout);
}

int editor_gui::draw(engine::renderer& pR)
{
	if (pR.is_key_down(engine::renderer::key_type::LControl)
	&&  pR.is_key_pressed(engine::renderer::key_type::E))
	{
		if (mLayout->isVisible())
			mLayout->hide();
		else
			mLayout->show();
	}

	mUpdate_timer += pR.get_delta();
	if (mUpdate_timer >= 0.5f)
	{
		const auto mouse_position_exact = pR.get_mouse_position();
		const auto mouse_position = pR.get_mouse_position(get_exact_position()) / get_unit();
		std::string position;
		position += "(";
		position += std::to_string(static_cast<int>(mouse_position_exact.x));
		position += ", ";
		position += std::to_string(static_cast<int>(mouse_position_exact.y));
		position += ")\n(";
		position += std::to_string(static_cast<int>(std::floor(mouse_position.x)));
		position += ", ";
		position += std::to_string(static_cast<int>(std::floor(mouse_position.y)));
		position += ")";
		mLb_mouse->setText(position);

		mLb_fps->setText("FPS: " + std::to_string(pR.get_fps()));

		mLb_scene->setText(mScene_name);

		mUpdate_timer = 0;
	}
	return 0;
}


editor::editor()
{
	mBlackout.set_color({ 0, 0, 0, 255 });
	mBlackout.set_size({ 1000, 1000 });
}

void editor::set_editor_gui(editor_gui & pEditor_gui)
{
	pEditor_gui.clear();
	mEditor_gui = &pEditor_gui;
	setup_editor(pEditor_gui);
}

void editor::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
}

// ##########
// scene_editor
// ##########

scene_editor::scene_editor()
{
	mBoundary_visualization.set_parent(*this);
	mTilemap_display.set_parent(*this);
}

bool scene_editor::open_scene(std::string pPath)
{
	mTilemap_manipulator.clean();
	mTilemap_display.clean();

	auto path = engine::encoded_path(pPath);
	if (!mLoader.load(path.parent(), path.filename()))
	{
		util::error("Unable to open scene '" + pPath + "'");
		return false;
	}

	auto texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, mLoader.get_tilemap_texture());
	if (!texture)
	{
		util::warning("Invalid tilemap texture in scene");
		util::info("If you have yet to specify a tilemap texture, you can ignore the last warning");
	}
	else
	{
		mTilemap_display.set_texture(texture);
		mTilemap_display.set_color({ 100, 100, 255, 150 });

		mTilemap_manipulator.load_tilemap_xml(mLoader.get_tilemap());
		mTilemap_manipulator.update_display(mTilemap_display);
	}

	mBoundary_visualization.set_boundary(mLoader.get_boundary());

	return true;
}

// ##########
// tilemap_editor
// ##########

tilemap_editor::tilemap_editor()
{
	set_depth(-1000);

	mPreview.set_color({ 255, 255, 255, 150 });
	mPreview.set_parent(*this);

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

	mTb_texture->setText(mLoader.get_tilemap_texture());

	update_tile_combobox_list();
	update_preview();
	update_labels();

	mTilemap_group->set_enabled(true);

	return true;
}

int tilemap_editor::draw(engine::renderer & pR)
{
	// Draw the black thing first
	mBlackout.draw(pR);

	// Editing is not allowed as there are no tiles to use.
	if (mTile_list.empty())
		return 1;

	const engine::fvector mouse_position = pR.get_mouse_position(mTilemap_display.get_exact_position());


	const engine::fvector tile_position_exact = mouse_position / get_unit();
	const engine::fvector tile_position
		= mCb_half_grid->isChecked()
		? engine::fvector(tile_position_exact * 2).floor() / 2
		: engine::fvector(tile_position_exact).floor();

	if (mState == state::none)
	{
		if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_left)
			&& pR.is_key_down(engine::renderer::key_type::LShift))
		{
			mState = state::drawing_region;
			mLast_tile = tile_position;
		}
		else if (pR.is_key_down(engine::renderer::key_type::LControl))
		{
			if (pR.is_key_pressed(engine::renderer::key_type::Z)) // Undo
			{
				mCommand_manager.undo();
				update_tilemap();
			}
			else if (pR.is_key_pressed(engine::renderer::key_type::Y)) // Redo
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
			copy_tile_type_at(tile_position_exact); // Copy tile (no need for a specific state for this)
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::Period))
		{
			next_tile();
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::Comma))
		{
			previous_tile();
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::Quote))
		{
			layer_up();
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::Slash))
		{
			layer_down();
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::R))
		{
			rotate_clockwise();
		}
	}

	switch (mState)
	{
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

	mBoundary_visualization.draw(pR);

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
		mTilemap_manipulator.clean();
		update_tilemap();
		return true;
	}, "- Clear the entire tilemap (Warning: Can't undo)");

	mTilemap_group->add_command("shift",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() < 2)
		{
			util::error("Not enough arguments");
			return false;
		}

		engine::fvector shift_amount;
		try {
			shift_amount = read_args_vector(pArgs);
		}
		catch (...)
		{
			util::error("Invalid offset input");
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
					util::error("Invalid layer input");
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
	}, "<X> <Y> [Layer#/current] - Shift the entire/layer of tilemap (Warning: Can't undo)");

	mTilemap_group->set_enabled(false);
	pTerminal.add_group(mTilemap_group);
}


void tilemap_editor::setup_editor(editor_gui & pEditor_gui)
{
	mCb_tile = pEditor_gui.add_combobox();
	mCb_tile->setItemsToDisplay(5);
	mCb_tile->connect("ItemSelected",
		[&]() {
			const int item = mCb_tile->getSelectedItemIndex();
			if (item == -1)
			{
				util::warning("No item selected");
			}
			mCurrent_tile = static_cast<size_t>(item);
			update_labels();
			update_preview();
		});

	mLb_layer = pEditor_gui.add_label("Layer: 0");
	mLb_rotation = pEditor_gui.add_label("Rotation: N/A");

	auto lb_texture = pEditor_gui.add_label("Texture:");
	lb_texture->setTextSize(14);

	mTb_texture = pEditor_gui.add_textbox();

	auto bt_apply_texture = pEditor_gui.add_button("Apply Texture");
	bt_apply_texture->connect("pressed", [&]() { apply_texture(); });

	mCb_half_grid = pEditor_gui.add_checkbox("Half Grid");
}

void tilemap_editor::copy_tile_type_at(engine::fvector pAt)
{
	const std::string atlas = mTilemap_manipulator.find_tile_name(pAt, mLayer);
	if (!atlas.empty())
		for (size_t i = 0; i < mTile_list.size(); i++) // Find tile in tile_list and set it as current tile
			if (mTile_list[i] == atlas)
			{
				mCurrent_tile = i;
				update_preview();
				update_labels();
				update_tile_combobox_selected();
				break;
			}
}

void tilemap_editor::draw_tile_at(engine::fvector pAt)
{
	assert(mTile_list.size() != 0);
	mTilemap_manipulator.explode_tile(pAt, mLayer);

	rpg::tile new_tile;
	new_tile.set_position(pAt);
	new_tile.set_atlas(mTile_list[mCurrent_tile]);
	new_tile.set_rotation(mRotation);

	std::shared_ptr<command_set_tiles> command
	(new command_set_tiles(mLayer, &mTilemap_manipulator));
	command->add(new_tile);

	mCommand_manager.execute(command);
	update_tilemap();
}

void tilemap_editor::erase_tile_at(engine::fvector pAt)
{
	mTilemap_manipulator.explode_tile(pAt, mLayer);

	std::shared_ptr<command_remove_tiles> command
		(new command_remove_tiles(mLayer, &mTilemap_manipulator));
	command->add(pAt);

	mCommand_manager.execute(command);

	mTilemap_manipulator.update_display(mTilemap_display);
	update_tilemap();
}

void tilemap_editor::next_tile()
{
	++mCurrent_tile %= mTile_list.size();
	update_tile_combobox_selected();
	update_preview();
	update_labels();
}

void tilemap_editor::previous_tile()
{
	mCurrent_tile = mCurrent_tile == 0 ? (mTile_list.size() - 1) : (mCurrent_tile - 1);
	update_tile_combobox_selected();
	update_preview();
	update_labels();
}

void tilemap_editor::layer_up()
{
	++mLayer;
	update_labels();
	update_highlight();
}

void tilemap_editor::layer_down()
{
	--mLayer;
	update_labels();
	update_highlight();
}

void tilemap_editor::rotate_clockwise()
{
	++mRotation %= 4;
	update_preview();
	update_labels();
}


void tilemap_editor::update_tile_combobox_list()
{
	assert(mCb_tile != nullptr);
	mCb_tile->removeAllItems();
	for (auto& i : mTile_list)
		mCb_tile->addItem(i);
	update_tile_combobox_selected();
}

void tilemap_editor::update_tile_combobox_selected()
{
	mCb_tile->setSelectedItemByIndex(mCurrent_tile);
}

void tilemap_editor::update_labels()
{
	assert(mLb_layer != nullptr);
	assert(mLb_rotation != nullptr);

	mLb_layer->setText(std::string("Layer: ") + std::to_string(mLayer));
	mLb_rotation->setText(std::string("Rotation: ") + std::to_string(mRotation));
}

void tilemap_editor::update_preview()
{
	if (!mTexture)
		return;
	mPreview.set_texture_rect(mTexture->get_entry(mTile_list[mCurrent_tile])->get_root_rect());

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
	mTilemap_manipulator.update_display(mTilemap_display);
	update_highlight();
}

void tilemap_editor::tick_highlight(engine::renderer& pR)
{
	if (pR.is_key_pressed(engine::renderer::key_type::RShift))
	{
		mIs_highlight = !mIs_highlight;
		update_highlight();
	}
}

void tilemap_editor::apply_texture()
{
	const std::string tilemap_texture_name = mTb_texture->getText();

	util::info("Applying tilemap Texture '" + tilemap_texture_name + "'...");
	auto new_texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, tilemap_texture_name);
	if (!new_texture)
	{
		util::error("Failed to load texture '" + tilemap_texture_name + "'");
		return;
	}

	mTexture = new_texture;

	mTilemap_display.set_texture(mTexture);
	update_tilemap();
	mTile_list = std::move(mTexture->compile_list());
	assert(mTile_list.size() != 0);

	mCurrent_tile = 0;

	mPreview.set_texture(mTexture);

	update_tile_combobox_list();
	update_preview();
	update_labels();

	mCurrent_texture_name = tilemap_texture_name;

	util::info("Tilemap texture applied");
}

int tilemap_editor::save()
{
	auto& doc = mLoader.get_document();
	auto ele_map = mLoader.get_tilemap();

	util::info("Saving tilemap...");

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

	util::info("Tilemap saved");

	return 0;
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
	mCommand_manager.clean();
	mTilemap_group->set_enabled(false);
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

collisionbox_editor::collisionbox_editor()
{
	set_depth(-1000);

	add_child(mTilemap_display);
	mWall_display.set_color({ 100, 255, 100, 200 });
	mWall_display.set_outline_color({ 255, 255, 255, 255 });
	mWall_display.set_outline_thinkness(1);
	add_child(mWall_display);

	mSelection_preview.set_color({ 0, 0, 0, 0 });
	mSelection_preview.set_outline_color({ 255, 255, 255, 100 });
	mSelection_preview.set_outline_thinkness(1);
	add_child(mSelection_preview);

	mCurrent_type = rpg::collision_box::type::wall;

	mState = state::normal;
}

bool collisionbox_editor::open_editor()
{
	mCommand_manager.clean();
	mSelection_preview.set_size({ get_unit(), get_unit() });
	return mContainer.load_xml(mLoader.get_collisionboxes());
}

int collisionbox_editor::draw(engine::renderer& pR)
{
	const engine::fvector mouse_position = pR.get_mouse_position(get_exact_position());
	const engine::fvector exact_tile_position = mouse_position / get_unit();
	const engine::fvector tile_position = engine::fvector(exact_tile_position).floor();

	switch (mState)
	{
	case state::normal:
	{
		// Select tile
		if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left))
		{
			if (!tile_selection(exact_tile_position)
				|| pR.is_key_down(engine::renderer::key_type::LShift)) // Left shift allows use to place wall on another wall
			{
				mSelection = mContainer.add_collision_box(mCurrent_type);

				mCommand_manager.add(std::shared_ptr<command_add_wall>(new command_add_wall(mSelection, &mContainer)));

				apply_wall_settings();
				mSelection->set_region({ tile_position, { 1, 1 } });

				mState = state::size_mode;
				mDrag_from = tile_position;
			}
			update_labels();
		}

		// Remove tile
		else if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right))
		{
			// No cycling when removing tile.
			if (tile_selection(exact_tile_position, false))
			{
				mCommand_manager.add(std::shared_ptr<command_remove_wall>(new command_remove_wall(mSelection, &mContainer)));

				mContainer.remove_box(mSelection);

				mSelection = nullptr;
				update_labels();
			}
		}

		else if (pR.is_key_down(engine::renderer::key_type::LControl))
		{
			if (pR.is_key_pressed(engine::renderer::key_type::Z)) // Undo
				mCommand_manager.undo();
			else if (pR.is_key_pressed(engine::renderer::key_type::Y)) // Redo
				mCommand_manager.redo();
		}

		break;
	}
	case state::size_mode:
	{
		// Size mode only last while user is holding down left-click
		if (!pR.is_mouse_down(engine::renderer::mouse_button::mouse_left))
		{
			mState = state::normal;
			update_labels();
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
		rect.set_size(resize_to - rect.get_offset() + engine::fvector(1, 1));

		// Update wall
		mSelection->set_region(rect);

		break;
	}
	}

	mBlackout.draw(pR);
	mTilemap_display.draw(pR);
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

	mSelection_preview.set_position(tile_position);
	mSelection_preview.draw(pR);

	mBoundary_visualization.draw(pR);

	return 0;
}

void collisionbox_editor::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mCollision_editor_group = std::make_shared<engine::terminal_command_group>();
	mCollision_editor_group->set_root_command("collision");
	mCollision_editor_group->add_command("clear",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mContainer.clean();
		mSelection.reset();
		return true;
	}, "- Clear all collision boxes (Warning: Can't undo)");

	mCollision_editor_group->add_command("offset",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (!mSelection)
		{
			util::error("Invalid selection");
			return false;
		}

		if (pArgs.size() < 1)
		{
			util::error("Not enough arguments");
			return false;
		}

		if (pArgs[0].get_raw() == "reset")
		{

		}

		if (pArgs.size() < 2)
		{
			util::error("Not enough arguments");
			return false;
		}

		engine::fvector offset;
		try {
			offset = read_args_vector(pArgs, mSelection->get_region().x, mSelection->get_region().y);
		}
		catch (...)
		{
			util::error("Invalid offset input");
			return false;
		}
		engine::frect changed = mSelection->get_region();
		changed.set_offset(changed.get_offset() + offset);
		mSelection->set_region(changed);

		return true;
	}, "<X> <Y> or reset - Offset collisionbox (Warning: Can't undo)");

	mCollision_editor_group->add_command("size",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (!mSelection)
		{
			util::error("Invalid selection");
			return false;
		}

		if (pArgs.size() < 2)
		{
			util::error("Not enough arguments");
			return false;
		}

		engine::fvector size;
		try {
			size = read_args_vector(pArgs, mSelection->get_region().w, mSelection->get_region().h);
		}
		catch (...)
		{
			util::error("Invalid offset input");
			return false;
		}

		engine::frect changed = mSelection->get_region();
		changed.set_size(size);
		mSelection->set_region(changed);

		return true;
	}, "<X> <Y> - Set size of collisionbox (Warning: Can't undo)");
	pTerminal.add_group(mCollision_editor_group);
}

int collisionbox_editor::save()
{
	util::info("Saving collision boxes");

	mContainer.generate_xml(mLoader.get_document(), mLoader.get_collisionboxes());

	mLoader.save();

	util::info("Saved " + std::to_string(mContainer.get_count()) +" collision box(es)");

	return 0;
}

void collisionbox_editor::setup_editor(editor_gui & pEditor_gui)
{
	// Collision box combobox
	mCb_type = pEditor_gui.add_combobox();
	mCb_type->addItem("Wall");
	mCb_type->addItem("Trigger");
	mCb_type->addItem("Button");
	mCb_type->addItem("Door");
	mCb_type->connect("ItemSelected",
		[&]() {
		const int item = mCb_type->getSelectedItemIndex();
		if (item == -1)
		{
			util::warning("No item selected");
			return;
		}

		// Set current tile
		mCurrent_type = static_cast<rpg::collision_box::type>(item); // Lazy cast
	});
	mCb_type->setSelectedItemByIndex(0);

	// Tile size label
	mLb_tilesize  = pEditor_gui.add_label("n/a, n/a");

	// Apply button

	// Wall group textbox
	pEditor_gui.add_small_label("Wall Group: ");
	mTb_wallgroup = pEditor_gui.add_textbox();

	// Wall size textbox
	pEditor_gui.add_small_label("Wall Size: ");
	mTb_size = pEditor_gui.add_textbox();

	auto bt_apply = pEditor_gui.add_button("Apply");
	bt_apply->connect("pressed", [&]() { apply_wall_settings(); });

	// Door related textboxes
	{
		mLo_door = pEditor_gui.add_sub_container();

		// Door name
		pEditor_gui.add_small_label("Door name:", mLo_door);
		mTb_door_name = pEditor_gui.add_textbox(mLo_door);

		// Door scene
		pEditor_gui.add_small_label("Door scene:", mLo_door);
		mTb_door_scene = pEditor_gui.add_textbox(mLo_door);

		// Door destination
		pEditor_gui.add_small_label("Door destination:", mLo_door);
		mTb_door_destination = pEditor_gui.add_textbox(mLo_door);

		// Door player offset
		pEditor_gui.add_small_label("Door player offset:", mLo_door);
		mTb_door_offsetx = pEditor_gui.add_textbox(mLo_door);
		mTb_door_offsety = pEditor_gui.add_textbox(mLo_door);

		// This container defaults invisible
		mLo_door->hide();
	}

}

void collisionbox_editor::apply_wall_settings()
{
	if (!mSelection)
		return;

	if (mSelection->get_type() != (rpg::collision_box::type)mCb_type->getSelectedItemIndex())
	{
		auto temp = mSelection;
		mContainer.remove_box(mSelection);
		auto new_box = mContainer.add_collision_box((rpg::collision_box::type)mCb_type->getSelectedItemIndex());
		new_box->set_region(temp->get_region());
		new_box->set_wall_group(temp->get_wall_group());
		mSelection = new_box;
		update_door_settings_labels();
	}


	// Apply wall group
	if (mTb_wallgroup->getText().isEmpty())
		mSelection->set_wall_group(nullptr);
	else
		mSelection->set_wall_group(mContainer.create_group(mTb_wallgroup->getText()));

	// Apply size
	engine::frect new_size;
	try {
		new_size = parsers::parse_attribute_rect<float>(mTb_size->getText());
		mSelection->set_region(new_size);
	}
	catch (...)
	{
		util::error("Failed to parse rect size");
	}

	// Apply door settings
	if (mSelection->get_type() == rpg::collision_box::type::door)
	{
		auto door = std::dynamic_pointer_cast<rpg::door>(mSelection);
		door->set_name       (mTb_door_name->getText());
		door->set_scene      (mTb_door_scene->getText());
		door->set_destination(mTb_door_destination->getText());
		door->set_offset     ({ util::to_numeral<float>(mTb_door_offsetx->getText())
			                  , util::to_numeral<float>(mTb_door_offsety->getText()) });
	}
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

void collisionbox_editor::update_labels()
{
	assert(mLb_tilesize != nullptr);

	if (!mSelection)
		return;

	const auto rect = mSelection->get_region();

	// Update size
	std::string size_text = std::to_string(rect.get_size().x);
	size_text += ", ";
	size_text += std::to_string(rect.get_size().y);
	mLb_tilesize->setText(size_text);

	// Update current wall group tb
	if (!mSelection->get_wall_group())
		mTb_wallgroup->setText("");
	else
		mTb_wallgroup->setText(mSelection->get_wall_group()->get_name());

	// Update current type
	mCurrent_type = mSelection->get_type();
	mCb_type->setSelectedItemByIndex(static_cast<size_t>(mCurrent_type));

	mTb_size->setText(parsers::generate_attribute_rect(mSelection->get_region()));

	update_door_settings_labels();
}

void collisionbox_editor::update_door_settings_labels()
{
	if (mSelection->get_type() != rpg::collision_box::type::door)
	{
		mLo_door->hide();
		return;
	}

	mLo_door->show();

	auto door = std::dynamic_pointer_cast<rpg::door>(mSelection);
	mTb_door_name->setText(door->get_name());
	mTb_door_scene->setText(door->get_scene());
	mTb_door_destination->setText(door->get_destination());
	mTb_door_offsetx->setText(std::to_string(door->get_offset().x));
	mTb_door_offsety->setText(std::to_string(door->get_offset().y));
}

// ##########
// atlas_editor
// ##########

atlas_editor::atlas_editor()
{
	mTexture.reset(new engine::texture);

	mFull_animation.set_color({ 100, 100, 255, 100 });
	mFull_animation.set_parent(mBackground);

	mSelected_firstframe.set_color({ 0, 0, 0, 0 });
	mSelected_firstframe.set_outline_color({ 255, 255, 0, 255 });
	mSelected_firstframe.set_outline_thinkness(1);
	mSelected_firstframe.set_parent(mBackground);

	mPreview_bg.set_anchor(engine::anchor::bottom);
	mPreview_bg.set_color({ 0, 0, 0, 200 });
	mPreview_bg.set_outline_color({ 255, 255, 255, 200 });
	mPreview_bg.set_outline_thinkness(1);
	mPreview_bg.set_parent(mBackground);

	mPreview.set_anchor(engine::anchor::bottom);
	mPreview.set_parent(mPreview_bg);
}

bool atlas_editor::open_editor()
{
	black_background();
	mPreview.set_visible(false);
	get_textures("./data/textures");
	if (!mTexture_list.empty())
	{
		mCb_texture_select->setSelectedItemByIndex(0);
		setup_for_texture(mTexture_list[0]);
	}
	return true;
}

int atlas_editor::draw(engine::renderer & pR)
{
	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left))
		atlas_selection(pR.get_mouse_position(mBackground.get_position()));

	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right))
		mDrag_offset = mBackground.get_position() - pR.get_mouse_position();
	else if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_right))
		mBackground.set_position(pR.get_mouse_position() + mDrag_offset);

	mBlackout.draw(pR);
	mBackground.draw(pR);

	for (auto& i : mAnimations)
	{
		auto full_region = i->animation->full_region();
		if (i == mSelection)
			mFull_animation.set_color({ 255, 255, 100, 50 });
		else
			mFull_animation.set_color({ 100, 100, 255, 100 });
		mFull_animation.set_position(full_region.get_offset());
		mFull_animation.set_size(full_region.get_size());
		mFull_animation.draw(pR);


		if (i == mSelection)
		{
			auto rect = i->animation->get_frame_at(i->animation->get_default_frame());
			mSelected_firstframe.set_position(rect.get_offset());
			mSelected_firstframe.set_size(rect.get_size());
			mSelected_firstframe.draw(pR);
		}
	}

	// Animation Preview
	if (mSelection && mSelection->animation->get_frame_count() > 1) // Only display if there is an animation
	{
		mPreview_bg.draw(pR);
		mPreview.draw(pR);
	}

	return 0;
}

int atlas_editor::save()
{
	if (mTexture_list.empty() || !mAtlas_changed)
		return 0;
	const std::string xml_path = mLoaded_texture.string() + ".xml";
	util::info("Saving atlas '" + xml_path + "'...");

	engine::texture_atlas atlas;
	for (auto& i : mAnimations)
	{
		if (i->name != "_Name_here_")
			atlas.add_entry(i->name, i->animation);
	}
	atlas.save(xml_path);
	mAtlas_changed = false;
	util::info("Atlas save");
	return 0;
}

void atlas_editor::get_textures(const std::string & pPath)
{
	for (auto& i : engine::fs::recursive_directory_iterator(pPath))
	{
		engine::encoded_path path = i.path().string();
		if (path.extension() == ".png")
		{
			mTexture_list.push_back(path.parent() / path.stem());

			if (engine::fs::exists(i.path().parent_path() / (i.path().stem().string() + ".xml")))
				mCb_texture_select->addItem(mTexture_list.back().filename());
			else
				mCb_texture_select->addItem("*" + mTexture_list.back().filename());
		}
	}
}

void atlas_editor::setup_for_texture(const engine::encoded_path& pPath)
{
	mAtlas_changed = false;
	mLoaded_texture = pPath;

	const std::string texture_path = pPath.string() + ".png";
	mTexture->unload();
	mTexture->set_texture_source(texture_path);
	mTexture->load();
	mPreview.set_texture(mTexture);
	mBackground.set_texture(mTexture);
	mBackground.set_texture_rect({ engine::fvector(0, 0), mTexture->get_size() });

	const std::string xml_path = pPath.string() + ".xml";
	if (!engine::fs::exists(xml_path))
	{
		//atlas.save(pPath.string() + ".xml");
		util::info("Generated new atlas");
		clear_gui();
		return;
	}

	mSelection = nullptr;
	mAnimations.clear();
	engine::texture_atlas atlas;
	atlas.load(pPath.string() + ".xml");
	if (!atlas.get_raw_atlas().empty())
	{
		for (auto& i : atlas.get_raw_atlas())
		{
			std::shared_ptr<editor_atlas_entry> entry(new editor_atlas_entry);
			entry->name = i.first;
			entry->animation = i.second.get_animation();
			mAnimations.push_back(entry);
		}
		mCb_entry_select->setSelectedItemByIndex(0);
		mSelection = mAnimations[0];
		update_settings();
		update_preview();
	}
	update_entry_list();
}

std::shared_ptr<atlas_editor::editor_atlas_entry> atlas_editor::find_animation(const std::string & pName)
{
	for (auto& i : mAnimations)
	{
		if (pName == i->name)
			return i;
	}
	return{};
}

void atlas_editor::new_entry()
{
	if (auto find = find_animation("_Name_here_"))
	{
		util::warning("A new, unnamed, entry has already been created");
		mSelection = find;
		update_settings();
		update_preview();
		return;
	}
	mSelection.reset(new editor_atlas_entry);
	mSelection->name = "_Name_here_";
	mSelection->animation.reset(new engine::animation);
	mSelection->animation->set_frame_count(1);
	mSelection->animation->set_loop(engine::animation::loop_type::none);
	mAnimations.push_back(mSelection);
	update_entry_list();
	update_settings();
	update_preview();
	mAtlas_changed = true;
}

void atlas_editor::remove_selected()
{
	for (size_t i = 0; i < mAnimations.size(); i++)
		if (mAnimations[i] == mSelection)
			mAnimations.erase(mAnimations.begin() + i);
	if (mAnimations.empty())
		mSelection = nullptr;
	else
		mSelection = mAnimations.back();
	update_entry_list();
	update_settings();
	update_preview();
	mAtlas_changed = true;
}

void atlas_editor::atlas_selection(engine::fvector pPosition)
{
	std::vector<std::shared_ptr<editor_atlas_entry>> hits;
	for (auto& i : mAnimations)
		if (i->animation->full_region().is_intersect(pPosition))
			hits.push_back(i);

	if (hits.empty())
		return;

	// Similar cycling as the collisionbox editor
	for (size_t i = 1; i < hits.size(); i++)
	{
		if (hits[i] == mSelection)
		{
			mSelection = hits[i - 1];
			update_settings();
			update_preview();
			return;
		}
	}
	mSelection = hits.back();
	update_settings();
	update_preview();
}

void atlas_editor::setup_editor(editor_gui & pEditor_gui)
{
	mCb_texture_select = pEditor_gui.add_combobox();
	mCb_texture_select->connect("ItemSelected",
		[&]() {
		const int item = mCb_texture_select->getSelectedItemIndex();
		if (item == -1)
		{
			util::warning("No item selected");
			return;
		}
		save();
		setup_for_texture(mTexture_list[item]);
	});

	pEditor_gui.add_small_label("Entry: ");
	mCb_entry_select = pEditor_gui.add_combobox();
	mCb_entry_select->connect("ItemSelected",
		[&]() {
		const int item = mCb_entry_select->getSelectedItemIndex();
		if (item == -1)
		{
			util::warning("No item selected");
			return;
		}
		mSelection = mAnimations[item];
		update_settings();
		update_preview();
	});

	auto bt_new = pEditor_gui.add_button("New");
	bt_new->connect("pressed",
		[&]() {
		new_entry();
	});

	pEditor_gui.add_small_label("Name: ");
	mTb_name = pEditor_gui.add_textbox();

	pEditor_gui.add_small_label("Frames: ");
	mTb_frames = pEditor_gui.add_textbox();

	pEditor_gui.add_small_label("Interval: ");
	mTb_interval = pEditor_gui.add_textbox();

	pEditor_gui.add_small_label("Default Frame: ");
	mTb_default_frame = pEditor_gui.add_textbox();

	pEditor_gui.add_small_label("Loop: ");
	mCb_loop = pEditor_gui.add_combobox();
	mCb_loop->addItem("No Loop");
	mCb_loop->addItem("Loop");
	mCb_loop->addItem("Pingpong Loop");
	mCb_loop->setSelectedItemByIndex(0);

	pEditor_gui.add_small_label("Size: ");
	mTb_size = pEditor_gui.add_textbox();

	auto bt_apply = pEditor_gui.add_button("Apply");
	bt_apply->connect("pressed", [&]() { apply_atlas_settings(); });

	auto bt_delete = pEditor_gui.add_button("Delete Entry");
	bt_delete->connect("pressed",
		[&]() {
		remove_selected();
	});

	auto bt_reload = pEditor_gui.add_button("Reload Image");
	bt_reload->connect("pressed",
		[&](){
		mTexture->unload();
		mTexture->load();
		mBackground.set_texture(mTexture);
		mBackground.set_texture_rect(engine::frect(engine::fvector(0, 0), mTexture->get_size()));
	});


	pEditor_gui.add_small_label("Background: ");
	mCb_bg_color = pEditor_gui.add_combobox();
	mCb_bg_color->addItem("Black");
	mCb_bg_color->addItem("White");
	mCb_bg_color->setSelectedItemByIndex(0);
	mCb_bg_color->connect("ItemSelected",
		[&]() {
		const int item = mCb_bg_color->getSelectedItemIndex();
		if (item == -1)
		{
			util::warning("No item selected");
			return;
		}
		if (item == 0)
		{
			black_background();
		}
		else if (item == 1)
		{
			white_background();
		}
	});
}

void atlas_editor::apply_atlas_settings()
{
	if (!mSelection)
	{
		util::error("Nothing selected");
		return;
	}

	// Rename
	if (mTb_name->getText() != mSelection->name
		&& util::shortcuts::validate_potential_xml_name(mTb_name->getText()))
	{
		if (!find_animation(mTb_name->getText()))
		{
			mSelection->name = mTb_name->getText();
			update_entry_list();
		}
		else
			util::error("Animation with name '" + mTb_name->getText() + "' already exists");
	}

	// Set loop
	mSelection->animation->set_loop(
		static_cast<engine::animation::loop_type>
		(mCb_loop->getSelectedItemIndex()));

	// Frame count
	try {
		mSelection->animation->set_frame_count(util::to_numeral<int>(mTb_frames->getText()));
	}
	catch (...)
	{
		util::error("Failed to parse frame count");
	}

	// Default Frame
	try {
		mSelection->animation->set_default_frame(util::to_numeral<int>(mTb_default_frame->getText()));
	}
	catch (...)
	{
		util::error("Failed to parse Default frame");
	}

	// Interval
	try{
		mSelection->animation->add_interval(0, util::to_numeral<float>(mTb_interval->getText()));
	}
	catch (...)
	{
		util::error("Failed to parse interval");
	}

	// Size and position
	try {
		engine::frect new_size = parsers::parse_attribute_rect<float>(mTb_size->getText());
		mSelection->animation->set_frame_rect(new_size);
	}
	catch (...)
	{
		util::error("Failed to parse rect size");
	}

	mAtlas_changed = true;
}

void atlas_editor::black_background()
{
	mBlackout.set_color({ 0, 0, 0, 255 });
	mPreview_bg.set_color({ 0, 0, 0, 150 });
	mPreview_bg.set_outline_color({ 255, 255, 255, 200 });
}

void atlas_editor::white_background()
{
	mBlackout.set_color({ 255, 255, 255, 255 });
	mPreview_bg.set_color({ 255, 255, 255, 150 });
	mPreview_bg.set_outline_color({ 0, 0, 0, 200 });
}


void editors::atlas_editor::update_entry_list()
{
	mCb_entry_select->removeAllItems();
	for (auto& i : mAnimations)
	{
		mCb_entry_select->addItem(i->name);
	}
	if (mSelection)
		mCb_entry_select->setSelectedItem(mSelection->name);
}

void atlas_editor::update_settings()
{
	if (!mSelection)
		return;
	mCb_entry_select->setSelectedItem(mSelection->name);
	mTb_name->setText(mSelection->name);
	mTb_frames->setText(std::to_string(mSelection->animation->get_frame_count()));
	mTb_default_frame->setText(std::to_string(mSelection->animation->get_default_frame()));
	mTb_interval->setText(std::to_string(mSelection->animation->get_interval()));
	mCb_loop->setSelectedItemByIndex(static_cast<size_t>(mSelection->animation->get_loop()));
	mTb_size->setText(parsers::generate_attribute_rect(mSelection->animation->get_frame_at(0)));
}

void atlas_editor::update_preview()
{
	if (!mSelection)
		return;

	auto position = mSelection->animation->get_frame_at(0).get_offset();
	position.x += mSelection->animation->full_region().w / 2;
	mPreview_bg.set_position(position);

	mPreview.set_animation(mSelection->animation);
	mPreview.restart();
	mPreview.start();

	mPreview_bg.set_size(mPreview.get_size());

}

void atlas_editor::clear_gui()
{
	mCb_entry_select->removeAllItems();
	mTb_frames->setText("0");
	mTb_interval->setText("0");
	mTb_size->setText("");
}


// ##########
// scroll_control_node
// ##########

void scroll_control_node::movement(engine::renderer & pR)
{
	float speed = pR.get_delta() * 4;
	engine::fvector position = get_position();
	if (pR.is_key_down(engine::renderer::key_type::Left))
		position += engine::fvector(1, 0)*speed;
	if (pR.is_key_down(engine::renderer::key_type::Right))
		position -= engine::fvector(1, 0)*speed;
	if (pR.is_key_down(engine::renderer::key_type::Up))
		position += engine::fvector(0, 1)*speed;
	if (pR.is_key_down(engine::renderer::key_type::Down))
		position -= engine::fvector(0, 1)*speed;
	set_position(position);
}

// ##########
// editor_manager
// ##########

editor_manager::editor_manager()
{
	set_depth(-1000);
	mTilemap_editor.set_parent(mRoot_node);
	mCollisionbox_editor.set_parent(mRoot_node);
	mCurrent_editor = nullptr;
}

bool editors::editor_manager::is_editor_open()
{
	return mCurrent_editor != nullptr;
}

void editor_manager::open_tilemap_editor(std::string pScene_path)
{
	util::info("Opening tilemap editor...");
	mTilemap_editor.set_editor_gui(mEditor_gui);
	if (mTilemap_editor.open_scene(pScene_path))
		mCurrent_editor = &mTilemap_editor;
	mTilemap_editor.open_editor();
	util::info("Editor loaded");
}

void editor_manager::open_collisionbox_editor(std::string pScene_path)
{
	util::info("Opening collisionbox editor...");
	mCollisionbox_editor.set_editor_gui(mEditor_gui);
	if (mCollisionbox_editor.open_scene(pScene_path))
		mCurrent_editor = &mCollisionbox_editor;
	mCollisionbox_editor.open_editor();
	util::info("Editor opened");
}

void editors::editor_manager::open_atlas_editor()
{
	util::info("Opening texture/atlas editor...");
	mAtlas_editor.set_editor_gui(mEditor_gui);
	mCurrent_editor = &mAtlas_editor;
	mAtlas_editor.open_editor();
	util::info("Editor opened");
}

void editor_manager::close_editor()
{
	if (!mCurrent_editor)
		return;

	util::info("Closing Editor...");
	mCurrent_editor->save();
	mCurrent_editor = nullptr;
	mEditor_gui.clear();
	util::info("Editor closed");
}

void editor_manager::set_world_node(node& pNode)
{
	mRoot_node.set_parent(pNode);
	mEditor_gui.set_parent(mRoot_node);
}

void editor_manager::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mTilemap_editor.set_resource_manager(pResource_manager);
	mCollisionbox_editor.set_resource_manager(pResource_manager);
}

void editor_manager::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mTilemap_editor.load_terminal_interface(pTerminal);
	mCollisionbox_editor.load_terminal_interface(pTerminal);
}

int editor_manager::draw(engine::renderer& pR)
{
	if (mCurrent_editor != nullptr)
	{
		mRoot_node.movement(pR);
		mCurrent_editor->draw(pR);
	}else
		mRoot_node.set_position({ 0, 0 });
	return 0;
}

void editor_manager::give_scene(std::string pScene_name)
{
    mEditor_gui.get_scene(pScene_name);
}

void editor_manager::refresh_renderer(engine::renderer & pR)
{
	mEditor_gui.set_renderer(pR);
	mEditor_gui.set_depth(-1001);
}

editor_boundary_visualization::editor_boundary_visualization()
{
	const engine::color line_color(255, 255, 255, 150);

	mLines.set_outline_color(line_color);
	mLines.set_color(engine::color(0, 0, 0, 0));
	mLines.set_parent(*this);
}

void editor_boundary_visualization::set_boundary(engine::frect pBoundary)
{
	// get_size requires pixel input
	const auto boundary = engine::scale(pBoundary, get_unit());

	mLines.set_position(boundary.get_offset());
	mLines.set_size(boundary.get_size());
}

int editor_boundary_visualization::draw(engine::renderer & pR)
{
	mLines.draw(pR);
	return 0;
}


