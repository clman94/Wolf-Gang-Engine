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
#include <array>

#include "vector.hpp"
#include "node.hpp"
#include "texture.hpp"
#include "time.hpp"
#include "rect.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "animation.hpp"
#include "resource.hpp"

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
	operator sf::Color() const
	{
		return{ r, g, b, a };
	}
#endif // SFML_COLOR_HPP

};

class render_object;

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
	int draw(render_object& pObject);

	int close();
	int add_object(render_object& pObject);
	int remove_object(render_object& pObject);
	void set_pixel_scale(float pScale);

	fvector get_size();

	// Resort all objects
	void request_resort();

	fvector get_mouse_position();
	fvector get_mouse_position(fvector pRelative);

	bool is_focused();

	int set_icon(const std::string& pPath);

	void set_visible(bool pVisible);
	void set_background_color(color pColor);

	float get_fps();

	float get_delta();

	void set_gui(tgui::Gui* pTgui);

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
	std::vector<render_object*> mObjects;
	bool mRequest_resort;
	frame_clock mFrame_clock;

	sf::Event mEvent;
	std::array<char, 256> mPressed_keys;
	std::array<char, 16> mPressed_buttons;
	void refresh_pressed();

	int draw_objects();
	void sort_objects();    // Sorts the mObjects array by depth
	void refresh_objects(); // Updates the indexs of all objects

	color mBackground_color;
};

class render_object :
	public node,
	public util::nocopy
{
public:
	render_object();
	~render_object();

	// Depth defines the order in which this object will be called
	void set_depth(depth_t pDepth);
	float get_depth();

	bool is_visible();
	void set_visible(bool pVisible);
	
	virtual int draw(renderer &pR) { return 0; }
	
	int is_rendered();

	void set_renderer(renderer& pR);
	void detach_renderer();

	friend class renderer;

protected:
	virtual void refresh_renderer(renderer& pR) {}

private:
	renderer* mRenderer;
	size_t mIndex;
	bool mVisible;
	depth_t mDepth;
};

class render_proxy
{
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

private:
	renderer* mR;
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
	public render_object
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
	public render_object
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

	void set_outline_color(color pColor)
	{
		shape.setOutlineColor(pColor);
	}

	void set_outline_thinkness(float pThickness)
	{
		shape.setOutlineThickness(pThickness);
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
	public render_object
{
public:
	sprite_node();
	void set_anchor(anchor pAnchor);
	virtual int draw(renderer &pR);
	void set_scale(fvector pScale);
	void set_texture(std::shared_ptr<texture> pTexture);
	std::shared_ptr<texture> get_texture() const;

	void set_texture_rect(const engine::frect& pRect);
	void set_color(color pColor);
	void set_rotation(float pRotation);
	fvector get_size();

private:
	std::shared_ptr<texture> mTexture;
	sf::Vertex mVertices[4];
	fvector mCenter;
	fvector mScale;
	float mRotation;
};

class font :
	public resource
{
public:
	void set_font_source(const std::string& pFilepath);
	void set_preferences_source(const std::string& pFilepath);
	bool load();
	bool unload();

private:
	bool load_preferences();

	std::string mFont_source;
	std::string mPreferences_source;

	std::unique_ptr<sf::Font> mSFML_font;
	int mCharacter_size;
	float mScale;
	friend class text_node;
};

class text_node :
	public render_object
{
public:
	text_node();
	void set_font(std::shared_ptr<font> pFont, bool pApply_preferences = false);

	void set_text(const std::string& pText);
	const std::string& get_text() const;

	void append_text(const std::string& pText);

	void set_character_size(int pPixels);
	void set_anchor(anchor pAnchor);
	void set_color(const color& pColor);
	void set_scale(float pScale);
	void copy_format(const text_node& pText_node);
	virtual int draw(renderer &pR);

private:
	std::shared_ptr<font> mFont;

	rectangle_node testrect;
	sf::Text mSfml_text;
	std::string mString; // Avoids reliance on sfml
	engine::anchor mAnchor;
	engine::fvector mOffset;

	void update_offset();
};


class animation_node :
	public render_object
{
public:
	animation_node();

	void set_frame(frame_t pFrame);
	void set_animation(std::shared_ptr<const engine::animation>, bool pSwap = false);
	bool set_animation(const std::string& pName, bool pSwap = false);
	void set_texture(std::shared_ptr<texture> pTexture);
	std::shared_ptr<texture> get_texture() const;

	engine::fvector get_size() const;

	bool tick(); // Returns true if the frame has changed.

	bool is_playing() const;
	void start();
	void pause();
	void stop();
	void restart();

	void set_color(color pColor);
	void set_anchor(anchor pAnchor);
	void set_rotation(float pRotation);

	int draw(renderer &r);

	float get_speed_scaler() const;
	void set_speed_scaler(float pScaler);

private:
	sprite_node mSprite;

	engine::clock  mClock;

	std::shared_ptr<const engine::animation> mAnimation;

	frame_t mFrame;

	anchor mAnchor;

	float mInterval;
	float mSpeed_scaler;
	bool mPlaying;

	void update_frame();
};


}

#endif