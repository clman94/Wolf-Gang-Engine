#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>

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

	bool is_key_pressed(key_type pKey_type, bool pIgnore_gui = false);
	bool is_key_down(key_type pKey_type, bool pIgnore_gui = false);
	bool is_mouse_pressed(mouse_button pButton_type, bool pIgnore_gui = false);
	bool is_mouse_down(mouse_button pButton_type, bool pIgnore_gui = false);

	int update_events();

	int initualize(ivector pSize, int pFps = 0);
	int draw();
	int draw(render_object& pObject);


	int close();
	int add_object(render_object& pObject);
	int remove_object(render_object& pObject);

	void set_target_size(fvector pSize);
	fvector get_target_size();

	// Resort all objects
	void request_resort();

	fvector get_mouse_position();
	fvector get_mouse_position(fvector pRelative);

	bool is_focused();

	int set_icon(const std::string& pPath);

	void set_window_title(const std::string& pTitle);
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

	tgui::Gui& get_tgui();

private:

	fvector mTarget_size;

	void refresh_view();
	void refresh_gui_view();

	tgui::Gui mTgui;
	bool mIs_mouse_busy;
	bool mIs_keyboard_busy;

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

class shader :
	public resource
{
public:
	bool load();
	bool unload();

	void set_vertex_path(const std::string& pPath);
	void set_fragment_path(const std::string& pPath);

#ifdef ENGINE_INTERNAL
	sf::Shader* get_sfml_shader()
	{
		if (!mSFML_shader)
			return nullptr;
		return mSFML_shader.get();
	}
#endif

private:
	std::string mVertex_shader_path;
	std::string mFragment_shader_path;
	std::unique_ptr<sf::Shader> mSFML_shader;
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
	render_proxy();

	void set_renderer(renderer& pR);
	renderer* get_renderer();
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
		: mBatch(nullptr),
		mRotation(0)
	{}

	vertex_reference(const vertex_reference& A);

	void set_position(fvector pPosition);
	fvector get_position();
	void set_texture_rect(frect pRect);
	void reset_size(fvector pSize);
	void hide();

	void set_rotation(int pRotation);
	int get_rotation() const;

	friend class vertex_batch;
private:
	int mRotation;
	engine::frect mTexture_rect;

	void update_rect();

	//int mRotation;
	vertex_batch* mBatch;
	size_t mIndex;

	sf::Vertex* get_reference();
};

class vertex_batch :
	public render_object
{
public:
	void set_texture(std::shared_ptr<texture> pTexture);
	vertex_reference add_quad(fvector pPosition, frect pTexture_rect, int pRotation = 0);
	int draw(renderer &pR);

	void set_color(color pColor);

	friend class vertex_reference;

private:
	std::vector<sf::Vertex> mVertices;
	std::shared_ptr<shader>  mShader;
	std::shared_ptr<texture> mTexture;
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

/* Something to consider
typedef fvector offset;
typedef fvector center;

class anchor_thing
{
public:
  anchor_thing();

  anchor_thing(anchor pAnchor);
  anchor_thing(offset pOffset);
	
  fvector calculate_offset();
  fvector calculate_offset(fvector pSize);

private:
  enum anchor_by
  {
    by_offset,
    by_center,
    by_anchor_point
  };

  fvector mPoint;
  anchor mAnchor;

  anchor_by mAnchor_by;
};
*/

class rectangle_node :
	public render_object
{
public:
	void set_color(const color& c);
	color get_color();
	void set_size(fvector s);
	fvector get_size();

	void set_outline_color(color pColor);

	void set_outline_thinkness(float pThickness);

	virtual int draw(renderer &pR);

private:
	std::shared_ptr<shader>  mShader;
	sf::RectangleShape shape;
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

	void set_shader(std::shared_ptr<shader> pShader);

private:
	std::shared_ptr<texture> mTexture;
	std::shared_ptr<shader>  mShader;
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

	void set_shader(std::shared_ptr<shader> pShader);

private:
	std::shared_ptr<font> mFont;
	std::shared_ptr<shader> mShader;

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

	void set_shader(std::shared_ptr<shader> pShader);

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