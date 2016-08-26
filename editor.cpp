
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

