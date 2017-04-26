#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

using namespace engine;

void font::set_font_source(const std::string & pFilepath)
{
	mFont_source = pFilepath;
}

void font::set_preferences_source(const std::string & pFilepath)
{
	mPreferences_source = pFilepath;
}

bool font::load()
{
	assert(!mFont_source.empty());

	if (!is_loaded())
	{
		mSFML_font.reset(new sf::Font);
		if (mPack)
		{
			mFont_data = mPack->read_all(mFont_source);
			mSFML_font->loadFromMemory(&mFont_data[0], mFont_data.size());
		}
		else
		{
			if (!mSFML_font->loadFromFile(mFont_source))
			{
				util::error("Failed to load font");
				return false;
			}
		}
		if (!load_preferences())
		{
			util::error("Failed to load font preferences");
			return false;
		}
		set_loaded(true);
	}
	return is_loaded();
}

bool font::unload()
{
	mSFML_font.reset();
	set_loaded(false);
	return true;
}

bool font::load_preferences()
{
	using namespace tinyxml2;
	XMLDocument doc;
	if (mPack)
	{
		const auto data = mPack->read_all(mPreferences_source);
		doc.Parse(&data[0], data.size());
	}
	else
	{
		if (doc.LoadFile(mPreferences_source.c_str()))
			return false;
	}

	auto ele_root = doc.FirstChildElement("font_preferences");
	if (!ele_root)
		return false;

	if (auto att_scale = ele_root->FloatAttribute("scale"))
		mScale = att_scale;
	else
		mScale = 1;

	if (auto att_size = ele_root->IntAttribute("size"))
		mCharacter_size = att_size;
	else
		mCharacter_size = 30;

	return true;
}

text_node::text_node()
{
	mAnchor = anchor::topleft;
}

void
text_node::set_font(std::shared_ptr<font> pFont, bool pApply_preferences)
{
	mFont = pFont;
	pFont->load();
	mSfml_text.setFont(*pFont->mSFML_font);
	mSfml_text.setScale({ pFont->mScale, pFont->mScale });
	mSfml_text.setCharacterSize(pFont->mCharacter_size);
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

const std::string& text_node::get_text() const
{
	return mString;
}

int
text_node::draw(renderer &pR)
{
	if (!mFont)
		return 1;

	const auto position = get_exact_position();

	// Remove annoying offset of text
	const engine::frect lbounds = mSfml_text.getLocalBounds();
	const engine::fvector loffset(0, lbounds.get_offset().y*mSfml_text.getScale().y);
	mSfml_text.setPosition(position + mOffset - loffset);

	/*engine::frect bounds = mSfml_text.getLocalBounds();
	testrect.set_color({ 100, 100, 100, 255 });
	testrect.set_position(position);
	testrect.set_size(bounds.get_size()*engine::fvector(mSfml_text.getScale()));
	testrect.draw(pR);*/

	if (mShader)
		pR.get_sfml_render().draw(mSfml_text, mShader->get_sfml_shader());
	else
		pR.get_sfml_render().draw(mSfml_text);
	return 0;
}

void text_node::set_shader(std::shared_ptr<shader> pShader)
{
	mShader = pShader;
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