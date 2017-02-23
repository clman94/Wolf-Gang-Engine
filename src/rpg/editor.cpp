
#include <rpg/editor.hpp>

using namespace editors;

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
	mTgui.add(mLayout);

	mLb_fps = std::make_shared<tgui::Label>();
	mLb_fps->setMaximumTextWidth(0);
	mLb_fps->setTextColor({ 200, 200, 200, 255 });
	mLb_fps->setText("FPS: N/A");
	mLayout->add(mLb_fps);

	mLb_mouse = tgui::Label::copy(mLb_fps);
	mLb_mouse->setText("(n/a, n/a)\n(n/a, n/a)");
	mLb_mouse->setTextSize(14);
	mLayout->add(mLb_mouse);

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

void editor_gui::refresh_renderer(engine::renderer & pR)
{
	pR.set_gui(&mTgui);
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

		mUpdate_timer = 0;
	}
	return 0;
}

editors::editor::editor()
{
	mBoundary_visualization.set_parent(*this);

	mTilemap_display.set_parent(*this);

	mBlackout.set_color({ 0, 0, 0, 255 });
	mBlackout.set_size({ 1000, 1000 });
}

bool editor::open_scene(std::string pPath)
{
	mTilemap_manipulator.clean();
	mTilemap_display.clean();

	if (!mLoader.load(pPath))
	{
		util::error("Unable to open scene '" + pPath + "'");
		return false;
	}

	auto texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, mLoader.get_tilemap_texture());
	if (!texture)
	{
		util::warning("Invalid tilemap texture in scene");
	}
	else
	{
		mTilemap_display.set_texture(texture);
		mTilemap_display.set_color({ 100, 100, 255, 150 });

		mTilemap_manipulator.load_tilemap_xml(mLoader.get_tilemap());
		mTilemap_manipulator.update_display(mTilemap_display);
	}

	mBoundary_visualization.set_boundary(mLoader.get_boundary());

	return editor_open();
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

bool tilemap_editor::editor_open()
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

	return true;
}

int tilemap_editor::draw(engine::renderer & pR)
{
	// Editing is not allowed it there are no tiles to use.
	if (mTile_list.empty())
		return 1;

	const engine::fvector mouse_position = pR.get_mouse_position(mTilemap_display.get_exact_position());

	const float half_tile_side = get_unit() / 2;
	const bool is_half_tile = (mPreview.get_size() == engine::fvector(half_tile_side, half_tile_side));

	const engine::fvector tile_position_exact = mouse_position / get_unit();
	const engine::fvector tile_position
		= (is_half_tile
			? engine::fvector(tile_position_exact * 2).floor() / 2
			: engine::fvector(tile_position_exact).floor());

	if (mState == state::none)
	{
		if (pR.is_mouse_down(engine::renderer::mouse_button::mouse_left)
			&& pR.is_key_down(engine::renderer::key_type::LShift))
		{
			mState = state::drawing_region;
			mLast_tile = tile_position;
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
		else if (pR.is_key_pressed(engine::renderer::key_type::SemiColon))
		{
			rotate_clockwise();
		}
		else if (pR.is_key_pressed(engine::renderer::key_type::L))
		{
			rotate_counter_clockwise();
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

	mBlackout.draw(pR);
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
}

void editors::tilemap_editor::copy_tile_type_at(engine::fvector pAt)
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
	mTilemap_manipulator.set_tile(pAt, mLayer, mTile_list[mCurrent_tile], mRotation);
	mTilemap_manipulator.update_display(mTilemap_display);
	update_highlight();
}

void tilemap_editor::erase_tile_at(engine::fvector pAt)
{
	mTilemap_manipulator.explode_tile(pAt, mLayer);
	mTilemap_manipulator.remove_tile(pAt, mLayer);
	mTilemap_manipulator.update_display(mTilemap_display);
	update_highlight();
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

void tilemap_editor::rotate_counter_clockwise()
{
	mRotation = mRotation == 0 ? 3 : (mRotation - 1);
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
	mTilemap_manipulator.update_display(mTilemap_display);
	mTile_list = std::move(mTexture->compile_list());
	assert(mTile_list.size() != 0);

	mCurrent_tile = 0;

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
	mTilemap_manipulator.condense_tiles();
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
}

// ##########
// collisionbox_editor
// ##########

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

bool collisionbox_editor::editor_open()
{
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
				apply_wall_settings();
				mSelection->set_region({ tile_position, { 1, 1 } });

				mState = state::size_mode;
				mDrag_from = tile_position;
			}
			update_labels();
		}

		// Remove tile
		if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right))
		{
			// No cycling when removing tile.
			if (tile_selection(exact_tile_position, false))
			{
				mContainer.remove_box(mSelection);
				mSelection = nullptr;
				update_labels();
			}
		}

		break;
	}
	case state::size_mode:
	{
		// Size mode only last while user is holding down left-click
		if (!pR.is_mouse_down(engine::renderer::mouse_button::mouse_left))
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
		}

		// Set current tile
		mCurrent_type = static_cast<rpg::collision_box::type>(item); // Lazy cast
	});
	mCb_type->setSelectedItemByIndex(0);

	// Tile size label
	mLb_tilesize  = pEditor_gui.add_label("n/a, n/a");

	// Apply button
	auto bt_apply = pEditor_gui.add_button("Apply");
	bt_apply->connect("pressed", [&]() { apply_wall_settings(); });

	// Wall group textbox
	auto lb_wall_group = pEditor_gui.add_label("Wall Group: ");
	lb_wall_group->setTextSize(10);
	mTb_wallgroup = pEditor_gui.add_textbox();

	// Door related textboxes
	{
		mLo_door = pEditor_gui.add_sub_container();

		// Door name
		auto lb_door_name = pEditor_gui.add_label("Door name:", mLo_door);
		lb_door_name->setTextSize(10);
		mTb_door_name = pEditor_gui.add_textbox(mLo_door);

		// Door scene
		auto lb_door_scene = pEditor_gui.add_label("Door scene:", mLo_door);
		lb_door_scene->setTextSize(10);
		mTb_door_scene = pEditor_gui.add_textbox(mLo_door);

		// Door destination
		auto lb_door_destination = pEditor_gui.add_label("Door destination:", mLo_door);
		lb_door_destination->setTextSize(10);
		mTb_door_destination = pEditor_gui.add_textbox(mLo_door);

		// Door player offset
		auto lb_door_offset = pEditor_gui.add_label("Door player offset:", mLo_door);
		lb_door_offset->setTextSize(10);
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

	// Apply wall group
	if (mTb_wallgroup->getText().isEmpty())
		mSelection->set_wall_group(nullptr);
	else
		mSelection->set_wall_group(mContainer.create_group(mTb_wallgroup->getText()));

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
	util::info("Editor loaded");
}

void editor_manager::open_collisionbox_editor(std::string pScene_path)
{
	util::info("Opening collisionbox editor...");
	mCollisionbox_editor.set_editor_gui(mEditor_gui);
	if (mCollisionbox_editor.open_scene(pScene_path))
		mCurrent_editor = &mCollisionbox_editor;
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

void editor_manager::refresh_renderer(engine::renderer & pR)
{
	mEditor_gui.set_renderer(pR);
	mEditor_gui.set_depth(-1001);
}

editor_boundary_visualization::editor_boundary_visualization()
{
	const engine::color line_color(255, 255, 255, 150);

	for (size_t i = 0; i < mLines.size(); i++)
	{
		mLines[i].set_color(line_color);
		mLines[i].set_parent(*this);
	}
}

void editor_boundary_visualization::set_boundary(engine::frect pBoundary)
{
	// get_size requires pixel input
	const auto boundary = engine::scale(pBoundary, get_unit());
	mLines[0].set_size({ boundary.w, 1 });
	mLines[1].set_size({ 1, boundary.h });
	mLines[2].set_size({ boundary.w, 1 });
	mLines[3].set_size({ 1, boundary.h });

	mLines[0].set_position(pBoundary.get_offset());
	mLines[1].set_position(pBoundary.get_offset());
	mLines[2].set_position({ 0, pBoundary.get_corner().y });
	mLines[3].set_position({ pBoundary.get_corner().x, 0 });
}

int editor_boundary_visualization::draw(engine::renderer & pR)
{
	for (auto& i : mLines)
		i.draw(pR);
	return 0;
}

