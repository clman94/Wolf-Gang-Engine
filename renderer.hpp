#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SFML\Graphics.hpp>
#include <TGUI\TGUI.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <list>
#include <map>
#include <unordered_map>
#include <cassert>

#include "vector.hpp"
#include "node.hpp"
#include "texture.hpp"
#include "ptr_GC.hpp"
#include "time.hpp"
#include "rect.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "animation.hpp"

namespace engine
{

struct color
{
	color_t r, g, b, a;
	color(
		color_t _r = 0,
		color_t _g = 0,
		color_t _b = 0,
		color_t _a = 255)
		: r(_r), g(_g), b(_b), a(_a)
	{}
#ifdef SFML_COLOR_HPP
	inline operator sf::Color() const
	{
		return{ r, g, b, a };
	}
#endif // SFML_COLOR_HPP

};

class render_client;

class renderer :
	public util::nocopy
{
public:
	renderer();
	~renderer();

	typedef sf::Keyboard::Key key_type;

	enum mouse_button
	{
		mouse_left,
		mouse_right,
		mouse_middle
	};

	bool is_key_pressed(key_type pKey_type);
	bool is_key_down(key_type pKey_type);
	bool is_mouse_pressed(mouse_button pButton_type);
	bool is_mouse_down(mouse_button pButton_type);

	int update_events();

	int initualize(ivector pSize, int pFps = 0);
	int draw();
	
	int close();
	int add_client(render_client& pClient);
	int remove_client(render_client& pClient);
	void set_pixel_scale(float pScale);

	fvector get_size();
	void request_resort();

	fvector get_mouse_position();
	fvector get_mouse_position(fvector pRelative);

	bool is_focused();

	int set_icon(const std::string& pPath);

	void set_visible(bool pVisible);
	void set_background_color(color pColor);

	float get_fps()
	{
		return mFrame_clock.get_fps();
	}

	float get_delta()
	{
		return mFrame_clock.get_delta();
	}

	void set_gui(tgui::Gui* pTgui)
	{
		mTgui = pTgui;
		pTgui->setWindow(mWindow);
	}

#ifdef ENGINE_INTERNAL

	/*sf::RenderWindow& get_sfml_window()
	{ return mWindow; }*/

	sf::RenderTarget& get_sfml_render()
	{ return mWindow; }

#endif

	friend class rectangle_node;

private:

	tgui::Gui* mTgui;

	sf::RenderWindow mWindow;
	std::vector<render_client*> mClients;
	bool mRequest_resort;
	frame_clock mFrame_clock;

	sf::Event mEvent;
	std::unordered_map<int, int> mPressed_keys;
	std::unordered_map<int, int> mPressed_buttons;
	void refresh_pressed();

	int draw_clients();
	void sort_clients();
	void refresh_clients();

	color mBackground_color;
};

class render_client :
	public util::nocopy
{
public:
	render_client();
	~render_client();
	void set_depth(depth_t pDepth);
	float get_depth();
	bool is_visible();
	void set_visible(bool pVisible);
	virtual int draw(renderer &pR) = 0;
	int is_rendered();

	void set_renderer(renderer& pR);
	void detach_renderer();

	friend class renderer;

protected:
	virtual void refresh_renderer(renderer& pR) {}

private:
	renderer* mRenderer;
	int mIndex;
	bool mVisible;
	depth_t mDepth;
};

class render_proxy
{
	renderer* mR;
public:
	render_proxy() : mR(nullptr)
	{
	}

	void set_renderer(renderer& pR)
	{
		mR = &pR;
		refresh_renderer(pR);
	}
	renderer* get_renderer()
	{
		return mR;
	}
protected:
	virtual void refresh_renderer(renderer& pR){}
};

class vertex_batch;

class vertex_reference
{
public:
	vertex_reference()
		: mBatch(nullptr)
	{}

	vertex_reference(const vertex_reference& A)
	{
		mBatch = A.mBatch;
		mIndex = A.mIndex;
	}

	void set_position(fvector pPosition);
	fvector get_position();
	void set_texture_rect(frect pRect, int rotation);
	void reset_size(fvector pSize);
	void hide();

	friend class vertex_batch;
private:
	//int mRotation;
	vertex_batch* mBatch;
	size_t mIndex;

	sf::Vertex* get_reference();
};

class vertex_batch :
	public render_client,
	public node
{
public:
	void set_texture(texture &pTexture);
	vertex_reference add_quad(fvector pPosition, frect pTexture_rect, int pRotation = 0);
	int draw(renderer &pR);

	void set_color(color pColor);

	friend class vertex_reference;

private:
	std::vector<sf::Vertex> mVertices;
	texture *mTexture;
};

enum struct anchor
{
	top = 0,
	topleft,
	topright,
	bottom,
	bottomleft,
	bottomright,
	left,
	right,
	center
};

template<typename T>
static vector<T> center_offset(const vector<T>& pSize, anchor pType)
{
	switch (pType)
	{
	case anchor::top:
		return{ pSize.x/2, 0 };
	case anchor::topleft:
		return{ 0, 0 };
	case anchor::topright:
		return{ pSize.x, 0 };
	case anchor::bottom:
		return{ pSize.x/2, pSize.y };
	case anchor::bottomleft:
		return{ 0, pSize.y };
	case anchor::bottomright:
		return pSize;
	case anchor::left:
		return{ 0, pSize.y/2 };
	case anchor::right:
		return{ pSize.x, pSize.y/2 };
	case anchor::center:
		return{ pSize.x/2, pSize.y/2 };
	}
	return 0;
}

template<typename T>
static vector<T> anchor_offset(const vector<T> pSize, anchor pType)
{
	return center_offset(pSize, pType) * -1;
}

class rectangle_node :
	public render_client,
	public node
{
	sf::RectangleShape shape;
public:
	void set_color(const color& c)
	{
		shape.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
	}
	color get_color()
	{
		auto c = shape.getFillColor();
		return{ c.r, c.g, c.b, c.a };
	}
	void set_size(fvector s)
	{
		shape.setSize({s.x, s.y});
	}
	fvector get_size()
	{
		return{ shape.getSize().x, shape.getSize().y };
	}

	virtual int draw(renderer &pR)
	{
		auto pos = get_exact_position();
		shape.setPosition({pos.x, pos.y});
		pR.mWindow.draw(shape);
		return 0;
	}
};

class sprite_node : 
	public render_client, 
	public node
{
public:
	sprite_node();
	void set_anchor(anchor pAnchor);
	virtual int draw(renderer &pR);
	void set_scale(fvector pScale);
	int set_texture(texture& pTexture);
	int set_texture(texture& pTexture, std::string pAtlas);
	void set_texture_rect(const engine::frect& pRect);
	void set_color(color pColor);
	void set_rotation(float pRotation);
	fvector get_size();

private:
	texture *mTexture;
	sf::Vertex mVertices[4];
	fvector mCenter;
	fvector mScale;
	float mRotation;
};

class font
{
	sf::Font sf_font;
public:
	int load(std::string pPath);
	friend class text_node;
};

// TODO
class text_format
{
public:
	void set_color(const color& pColor)
	{ mColor = pColor; }

	void set_scale(float pScale)
	{ mScale = pScale; }

	void set_font(font &pFont)
	{ mFont = &pFont; }

	void set_character_size(unsigned int pPixels)
	{ mCharacter_size = pPixels; }

public:
	float mScale;
	color mColor;
	font *mFont;
	unsigned int mCharacter_size;
};

class text_node :
	public render_client,
	public node
{
public:
	text_node();
	void set_font(font& pFont);
	void set_text(const std::string& pText);
	void append_text(const std::string& pText);
	std::string get_text();
	void set_character_size(int pPixels);
	void set_anchor(anchor pAnchor);
	void set_color(const color& pColor);
	void set_scale(float pScale);
	void copy_format(const text_node& pText_node);
	virtual int draw(renderer &pR);

private:
	rectangle_node testrect;
	sf::Text mSfml_text;
	std::string mString; // Avoids reliance on sfml
	engine::anchor mAnchor;
	engine::fvector mOffset;

	void update_offset();
};

class rich_text_node :
	public render_client,
	public node
{
	struct text_block
	{
		sf::Text sfml_text;
		text_format format;
	};
	std::list<text_block> mText_blocks;
public:
	// TODO
};

class animation_node :
	public render_client,
	public node
{
public:
	animation_node();

	void set_frame(frame_t pFrame);
	void set_animation(const animation& pAnimation, bool pSwap = false);
	void set_texture(texture& pTexture);

	engine::fvector get_size();

	int tick();

	bool is_playing();
	void start();
	void pause();
	void stop();
	void restart();

	void set_color(color pColor);
	void set_anchor(anchor pAnchor);
	void set_rotation(float pRotation);

	int draw(renderer &r);

private:
	sprite_node mSprite;

	engine::clock  mClock;

	const animation* mAnimation;

	frame_t mCount;
	frame_t mFrame;

	anchor mAnchor;

	int mInterval;
	bool mPlaying;
};


}

#endif