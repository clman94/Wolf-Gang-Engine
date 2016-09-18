
#include "editor.hpp"

using namespace editor;

void tgui_list_layout::updateWidgetPositions()
{
	auto widgets = getWidgets();

	Widget::Ptr last;
	for (auto i : widgets)
	{
		if (last)
		{
			auto pos = last->getPosition();
			pos.y += last->getSize().y;
			i->setPosition(pos.x, pos.y);
		}

		last = i;
	}
}

void editor_gui::initualize()
{
	mLayout = std::make_shared<tgui_list_layout>();
	mLayout->setSize(300, 500);
	mLayout->setBackgroundColor({ 0, 0, 0, 90 });
	mLayout->hide();
	mTgui.add(mLayout);

	mLb_mode = std::make_shared<tgui::Label>();
	mLb_mode->setMaximumTextWidth(0);
	mLb_mode->setTextColor({ 200, 200, 200, 255 });
	mLb_mode->setText("Mode: Normal Play");
	mLayout->add(mLb_mode);

	mLb_fps = tgui::Label::copy(mLb_mode);
	mLb_fps->setText("FPS: n/a");
	mLayout->add(mLb_fps);

	mLb_mouse = tgui::Label::copy(mLb_mode);
	mLb_mouse->setText("(n/a, n/a) : (n/a, n/a)");
	mLb_mouse->setTextSize(14);
	mLayout->add(mLb_mouse);

	mEditor_layout = std::make_shared<tgui_list_layout>();
	mEditor_layout->setSize(mLayout->getSize().x, 400);
	mEditor_layout->setBackgroundColor({ 0, 0, 0, 0 });
	mLayout->add(mEditor_layout);
}

void editor_gui::clear()
{
	mEditor_layout->removeAllWidgets();
}

tgui::Label::Ptr editor_gui::add_label(const std::string & pText)
{
	auto nlb = tgui::Label::copy(mLb_mode);
	nlb->setText(pText);
	nlb->setTextStyle(sf::Text::Bold);
	mEditor_layout->add(nlb);
	return nlb;
}

void editor_gui::refresh_renderer(engine::renderer & pR)
{
	pR.set_gui(&mTgui);
}

int editor_gui::draw(engine::renderer& pR)
{
	tick(pR, mCamera_offset);
	
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
		auto mouse_position = pR.get_mouse_position(mCamera_offset);
		std::string position;
		position += "(";
		//position += std::to_string(mouse_position.x);
		//position += ", ";
		//position += std::to_string(mouse_position.y);
		//position += ") : (";
		position += std::to_string(static_cast<int>(std::floor(mouse_position.x / 32)));
		position += ", ";
		position += std::to_string(static_cast<int>(std::floor(mouse_position.y / 32)));
		position += ")";
		mLb_mouse->setText(position);

		mLb_fps->setText("FPS: " + std::to_string(pR.get_fps()));

		mUpdate_timer = 0;
	}
	return 0;
}

tilemap_editor::tilemap_editor()
{
	set_depth(-1000);

	mTilemap_display.set_depth(-1001);

	mBlackout.set_color({ 0, 0, 0, 255 });
	mBlackout.set_size({ 1000, 1000 });
	mBlackout.set_depth(-1000);

	mPreview.set_color({ 255, 255, 255, 100 });

	mCurrent_tile = 0;
	mRotation = 0;
	mLayer = 0;
}

int tilemap_editor::open_scene_tilemap(const std::string & pPath)
{
	if (mScene_xml.LoadFile(pPath.c_str()))
	{
		util::error("Could not load scene tilemap to edit");
	}
	mPath = pPath;

	auto ele_root = mScene_xml.RootElement();
	mMap_xml = ele_root->FirstChildElement("map");

	if (mMap_xml)
	{
		// Load tilemap texture
		if (auto ele_texture = mMap_xml->FirstChildElement("texture"))
		{
			mTexture = mTexture_manager->get_texture(ele_texture->GetText());
			if (!mTexture){
				util::error("Invalid tilemap texture");
				return 1;
			}
			mTilemap_display.set_texture(*mTexture);

			mTilemap_loader.load_tilemap_xml(mMap_xml);
			mTilemap_loader.update_display(mTilemap_display);

			mTile_list = std::move(mTexture->compile_list());
			assert(mTile_list.size() != 0);

			update_preview();
			update_labels();
		}
		else
			util::error("Tilemap texture is not defined");
	}
	return 0;
}

int tilemap_editor::draw(engine::renderer & pR)
{
	movement(pR);

	if (pR.is_key_down(engine::renderer::key_type::Return))
		save();

	auto mouse_position = pR.get_mouse_position();
	engine::fvector tile_position = ((mouse_position - mTilemap_display.get_position()) / 32).floor();

	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_left))
	{
		assert(mTile_list.size() != 0);
		mTilemap_loader.explode_tile(tile_position, mLayer);
		mTilemap_loader.set_tile(tile_position, mLayer, mTile_list[mCurrent_tile], mRotation);
		mTilemap_loader.update_display(mTilemap_display);
	}

	if (pR.is_mouse_pressed(engine::renderer::mouse_button::mouse_right))
	{
		assert(mTile_list.size() != 0);

		mTilemap_loader.explode_tile(tile_position, mLayer);
		mTilemap_loader.remove_tile(tile_position, mLayer);
		mTilemap_loader.update_display(mTilemap_display);
	}

	if (pR.is_key_pressed(engine::renderer::key_type::Period)) // Next tile
	{
		++mCurrent_tile %= mTile_list.size();
		mPreview.set_texture(*mTexture, mTile_list[mCurrent_tile]);
		update_preview();
		update_labels();
	}

	if (pR.is_key_pressed(engine::renderer::key_type::Comma)) // Previous tile
	{
		mCurrent_tile = mCurrent_tile == 0 ? (mTile_list.size() - 1) : (mCurrent_tile - 1);
		update_preview();
		update_labels();
	}

	if (pR.is_key_pressed(engine::renderer::key_type::Quote))
	{
		++mLayer;
		update_labels();
	}

	if (pR.is_key_pressed(engine::renderer::key_type::Slash))
	{
		--mLayer;
		update_labels();
	}

	if (pR.is_key_pressed(engine::renderer::key_type::SemiColon))
	{
		++mRotation %= 4;
		update_preview();
		update_labels();
	}

	if (pR.is_key_pressed(engine::renderer::key_type::L))
	{
		mRotation = mRotation == 0 ? 3 : (mRotation - 1);
		update_preview();
		update_labels();
	}

	mBlackout.draw(pR);
	mTilemap_display.draw(pR);

	mPreview.set_position(tile_position * 32
		+ mTilemap_display.get_position());
	mPreview.draw(pR);

	return 0;
}

void tilemap_editor::set_texture_manager(rpg::texture_manager & pTexture_manager)
{
	mTexture_manager = &pTexture_manager;
}

void tilemap_editor::setup_controls(editor_gui & pEditor_gui)
{
	mLb_layer = pEditor_gui.add_label("Layer: 0");
	mLb_tile = pEditor_gui.add_label("Tile: N/A");
	mLb_rotation = pEditor_gui.add_label("Rotation: N/A");
}

void tilemap_editor::movement(engine::renderer & pR)
{
	float delta = pR.get_delta() * 128;
	engine::fvector position = mTilemap_display.get_position();


	if (pR.is_key_down(engine::renderer::key_type::Left))
		position += engine::fvector(1, 0)*delta;
	if (pR.is_key_down(engine::renderer::key_type::Right))
		position -= engine::fvector(1, 0)*delta;
	if (pR.is_key_down(engine::renderer::key_type::Up))
		position += engine::fvector(0, 1)*delta;
	if (pR.is_key_down(engine::renderer::key_type::Down))
		position -= engine::fvector(0, 1)*delta;
	mTilemap_display.set_position(position);
}

void tilemap_editor::update_labels()
{
	assert(mLb_tile != nullptr);
	assert(mLb_layer != nullptr);
	assert(mLb_rotation != nullptr);

	mLb_layer->setText(std::string("Layer: ") + std::to_string(mLayer));
	mLb_tile->setText(std::string("Tile: ") + mTile_list[mCurrent_tile]);
	mLb_rotation->setText(std::string("Rotation: ") + std::to_string(mRotation));
}

void tilemap_editor::update_preview()
{
	mPreview.set_texture(*mTexture, mTile_list[mCurrent_tile]);
	mPreview.set_rotation(90.f * mRotation);
	mPreview.set_anchor(engine::anchor::topleft);
}

void tilemap_editor::save()
{
	auto ele_layer = mMap_xml->FirstChildElement("layer");
	while (ele_layer)
	{
		ele_layer->DeleteChildren();
		mScene_xml.DeleteNode(ele_layer);
		ele_layer = mMap_xml->FirstChildElement("layer");
	}
	mTilemap_loader.condense_tiles();
	mTilemap_loader.generate(mScene_xml, mMap_xml);
	mScene_xml.SaveFile(mPath.c_str());
}

