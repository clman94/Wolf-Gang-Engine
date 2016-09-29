#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

using namespace engine;

text_node::text_node()
{
	mAnchor = anchor::topleft;
}

int
font::load(std::string pPath)
{
	return sf_font.loadFromFile(pPath);
}

void
text_node::set_font(font& pFont)
{
	mSfml_text.setFont(pFont.sf_font);
	update_offset();
}

void 
text_node::set_text(const std::string& pText)
{
	mSfml_text.setString(pText);
	mString = pText;
	update_offset();
}

void
text_node::append_text(const std::string& pText)
{
	set_text(get_text() + pText);
}

std::string
text_node::get_text()
{
	return mString;
}

int
text_node::draw(renderer &pR)
{
	auto position = get_exact_position();

	// Remove annoying offset of text
	engine::frect lbounds = mSfml_text.getLocalBounds();
	engine::fvector loffset(0, lbounds.get_offset().y*mSfml_text.getScale().y);
	mSfml_text.setPosition(position + mOffset - loffset);

	engine::frect bounds = mSfml_text.getLocalBounds();
	testrect.set_color({ 100, 100, 100, 255 });
	testrect.set_position(position);
	testrect.set_size(bounds.get_size()*engine::fvector(mSfml_text.getScale()));
	testrect.draw(pR);

	pR.get_sfml_render().draw(mSfml_text);
	return 0;
}

void text_node::update_offset()
{
	engine::frect bounds = mSfml_text.getGlobalBounds();
	mOffset = anchor_offset<float>(bounds.get_size(), mAnchor);
	//mOffset -= bounds.get_offset() - get_exact_position();
}

void
text_node::set_character_size(int pPixels)
{
	mSfml_text.setCharacterSize(pPixels);
	update_offset();
}

void
text_node::set_anchor(engine::anchor pAnchor)
{
	mAnchor = pAnchor;
	update_offset();
}

void
text_node::set_scale(float pScale)
{
	mSfml_text.setScale({ pScale, pScale });
}

void
text_node::copy_format(const text_node& pText_node)
{
	auto nfont = pText_node.mSfml_text.getFont();
	if (nfont) mSfml_text.setFont(*nfont);

	mSfml_text.setFillColor(pText_node.mSfml_text.getFillColor());
	mSfml_text.setCharacterSize(pText_node.mSfml_text.getCharacterSize());
	mSfml_text.setScale(pText_node.mSfml_text.getScale());
	mSfml_text.setStyle(pText_node.mSfml_text.getStyle());
	update_offset();
}


void 
text_node::set_color(const color& pColor)
{
	mSfml_text.setFillColor(pColor);
}
