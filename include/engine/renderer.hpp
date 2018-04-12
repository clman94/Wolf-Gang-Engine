#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <cassert>
#include <array>

#include "vector.hpp"
#include "node.hpp"
#include "texture.hpp"
#include "time.hpp"
#include "rect.hpp"
#include "utility.hpp"
#include "animation.hpp"
#include "resource.hpp"
#include "color.hpp"

namespace engine
{

class display_window
{
public:
	display_window() {}
	display_window(const std::string& pTitle, ivector pSize);
	~display_window();
	void initualize(const std::string& pTitle, ivector pSize);
	void set_size(ivector pSize);
	ivector get_size() const;

	void fullscreen_mode();
	void windowed_mode();
	void toggle_mode();
	bool is_fullscreen() const;

	void set_title(const std::string& pTitle);

	int set_icon(const std::string& pPath);
	int set_icon(const std::vector<char>& pData);

	bool is_open() const;

	// Returns false when window is closed
	bool poll_events();

	// Update and display the window
	void display();

	// Clear all contents for rendering a new frame
	void clear();

	void push_events_to_imgui() const;

	sf::RenderWindow& get_sfml_window();

private:
	sf::RenderWindow mWindow;
	bool mIs_fullscreen;
	ivector mSize;
	std::string mTitle;

	// Storing the events allows multiple renderers per window.
	// This feature is mostly meant for the editors.
	std::vector<sf::Event> mEvents;

	friend class renderer;
};

class renderer;

class render_object :
	public node,
	public util::nocopy
{
public:
	render_object();
	~render_object();

	// Depth defines the order in which this object will be called
	void set_depth(float pDepth);
	float get_depth();

	bool is_visible();

	// Sets wether or not this object is renderer. draw() will not be called if false.
	void set_visible(bool pVisible);

	virtual int draw(renderer &pR) { return 0; }

	virtual frect get_render_rect() const { return{}; }

	void set_renderer(renderer& pR, bool pManual_render = false);
	renderer* get_renderer() const;
	void detach_renderer();
	friend class renderer;

protected:
	virtual void refresh_renderer(renderer& pR) {}

private:
	renderer * mRenderer;
	bool mVisible;
	float mDepth;
	bool mManual_render;
};

class renderer :
	public util::nocopy
{
public:
	renderer();
	~renderer();

	typedef sf::Keyboard::Key key_code;

	enum mouse_button
	{
		mouse_left,
		mouse_right,
		mouse_middle
	};

	bool is_key_pressed(key_code pKey_type, bool pIgnore_gui = false);
	bool is_key_down(key_code pKey_type, bool pIgnore_gui = false);
	bool is_mouse_pressed(mouse_button pButton_type, bool pIgnore_gui = false);
	bool is_mouse_down(mouse_button pButton_type, bool pIgnore_gui = false);

	void update_events(); // Call the window's poll_event method first
	void update_events(display_window& pWindow);
	void update_events(display_window& pWindow, engine::fvector pMouse_position); // Same as above but you can override the mouse position (useful for editor)

	int draw();
	int draw(render_object& pObject);
	
	bool add_object(render_object& pObject);
	bool remove_object(render_object& pObject);

	void set_target_size(fvector pSize);
	fvector get_target_size() const;

	// Resort all objects
	void request_resort();

	fvector get_mouse_position() const;
	fvector get_mouse_position(fvector pRelative) const;
	fvector get_mouse_position(const node& pNode) const;

	// Converts vector relative to window to coords relative to the view and its scale.
	fvector window_to_game_coords(ivector pPixels) const;
	fvector window_to_game_coords(ivector pPixels, const fvector& pRelative) const;
	fvector window_to_game_coords(ivector pPixels, const node& pNode) const; // Relative to node

	bool is_focused();

	void set_visible(bool pVisible);
	void set_background_color(color pColor);

	float get_fps() const;
	float get_delta() const;

	void set_window(display_window& pWindow);
	display_window* get_window() const;

	void set_target_render(sf::RenderTarget& pTarget);

	void refresh();

#ifdef ENGINE_INTERNAL
	sf::RenderTarget& get_sfml_render()
	{
		assert(mRender_target);
		return *mRender_target;
	}
#endif

	// Get the ascii text inputted last event handling
	std::string get_entered_text() const;

	// Get the unicode text inputted last event handling
	std::u32string get_entered_text_unicode() const;

private:
	std::u32string mEntered_text;

	bool is_mouse_within_target() const;

	fvector mTarget_size;

	void refresh_view();

	sf::View mView;
	display_window* mWindow;
	sf::RenderTarget* mRender_target;

	std::vector<render_object*> mObjects;
	bool mRequest_resort;
	frame_clock mFrame_clock;

	enum class input_state
	{
		none,
		pressed,
		hold,
		released,
	};

	std::array<input_state, 256> mPressed_keys;
	std::array<input_state, 16> mPressed_buttons;

	ivector mMouse_position;

	void refresh_input();

	int draw_objects();
	void sort_objects();    // Sorts the mObjects array by depth

	color mBackground_color;

	friend class scoped_clipping;
};

class scoped_clipping
{
public:
	scoped_clipping(renderer& pRenderer, frect pRect)
	{
		mOrig_view = pRenderer.mView;

		sf::View new_view(pRect);
		frect viewport;
		//viewport.x = 
	}

private:
	sf::View mOrig_view;
	renderer* mRenderer;
};

const std::string shader_restype = "shader";

class shader :
	public resource
{
public:
	bool load() override;
	bool unload() override;

	void set_vertex_path(const std::string& pPath);
	void set_fragment_path(const std::string& pPath);

	const std::string& get_type() const override
	{
		return shader_restype;
	}

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
		mRotation(0),
		mHskew(0)
	{}

	void set_position(fvector pPosition);
	fvector get_position();
	void set_size(fvector pSize);
	fvector get_size() const;

	void set_texture_rect(frect pRect);
	void hide();

	void set_rotation(int pRotation);
	int get_rotation() const;

	void set_hskew(float pPercent);

	void set_color(const color& pColor);


	friend class vertex_batch;
private:
	int mRotation;
	float mHskew;
	frect mRect;
	color mColor;
	engine::frect mTexture_rect;

	void update_position();
	void update_texture();
	void update_color();

	vertex_batch* mBatch;
	size_t mIndex;
	
	// TODO: Flipping
	//bool mH_flip;
	//bool mV_flip;

	sf::Vertex* get_reference();
};

class vertex_batch :
	public render_object
{
public:
	vertex_batch();

	void set_texture(std::shared_ptr<texture> pTexture);
	vertex_reference add_quad(fvector pPosition, frect pTexture_rect, int pRotation = 0);
	void reserve_quads(size_t pAmount);
	
	int draw(renderer &pR);

	void use_render_texture(bool pEnable);

#ifdef ENGINE_INTERNAL

	int draw(renderer &pR, const sf::Texture& pTexture);

#endif

	void clean();

	void set_color(color pColor);

	friend class vertex_reference;

private:
	bool mUse_render_texture;

	void update_texture(renderer& pR);

	virtual void refresh_renderer(renderer& pR);

	std::vector<sf::Vertex> mVertices;
	std::shared_ptr<shader>  mShader;
	std::shared_ptr<texture> mTexture;
	sf::RenderTexture mRender;
};

class grid :
	public render_object
{
public:
	grid();
	~grid();
	void set_major_size(fvector pSize);
	void set_sub_grids(int pAmount);

	void update_grid(renderer &pR);

	int draw(renderer &pR);
private:
	void add_grid(int x, int y, fvector pCell_size, sf::Color pColor);
	void add_line(fvector pV0, fvector pV1, sf::Color pColor);

	fvector mMajor_size;
	int mSub_grids;

	std::vector<sf::Vertex> mVertices;
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
	return -center_offset(pSize, pType);
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
	rectangle_node();

	void set_anchor(anchor pAnchor);

	void set_color(const color& c);
	color get_color();
	void set_size(fvector s);
	fvector get_size() const;

	void set_outline_color(color pColor);

	void set_outline_thinkness(float pThickness);

	virtual int draw(renderer &pR);

	virtual frect get_render_rect() const;

private:
	anchor mAnchor;
	std::shared_ptr<shader> mShader;
	sf::RectangleShape shape;
};

class sprite_node : 
	public render_object
{
public:
	sprite_node();
	void set_anchor(anchor pAnchor);
	virtual int draw(renderer &pR);

	void set_texture(std::shared_ptr<texture> pTexture);
	std::shared_ptr<texture> get_texture() const;

	void set_texture_rect(const engine::frect& pRect);
	void set_color(color pColor);
	fvector get_size() const;

	void set_shader(std::shared_ptr<shader> pShader);

	virtual frect get_render_rect() const;

private:
	std::shared_ptr<texture> mTexture;
	std::shared_ptr<shader>  mShader;
	sf::Vertex mVertices[4];
	frect mRect;
	fvector mCenter;
	anchor mAnchor;
};

static const std::string font_restype = "font";
class font :
	public resource
{
public:

	void set_font_source(const std::string& pFilepath);
	void set_preferences_source(const std::string& pFilepath);
	bool load() override;
	bool unload() override;

	const std::string& get_type() const override
	{
		return font_restype;
	}

private:
	bool load_preferences();

	std::string mFont_source;
	std::string mPreferences_source;

	std::vector<char> mFont_data; // Stores data loaded from a pack

	std::unique_ptr<sf::Font> mSFML_font;
	int mCharacter_size;
	fvector mOffset;
	friend class text_node;
	friend class formatted_text_node;
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

class text_format
{
public:
	enum format : uint32_t
	{
		none      = 0,
		bold      = 1 << 0,
		italics   = 1 << 1,
		wave      = 1 << 2,
		shake     = 1 << 3,
		rainbow   = 1 << 4,
	};

	struct block
	{
		std::string mText;
		std::string mMeta;
		color       mColor;
		uint32_t    mFormat;

		/*struct format_options // TODO: More customization
		{
			float wave_height;
			float wave_speed;
			float shake_amount;
		};*/
	};

	text_format();
	text_format(const char* pText);
	text_format(const std::string& pText);

	/*
	Basic Format

	Bold
	<b></b>

	Italics
	<i></i>

	Color
	<c hex="RRGGBBAA"></c>
	<c r="255" g="255" b="255" a="255"></c>

	Special Effects
	<wave></wave>
	<shake></shake>
	<rainbow></rainbow>
	*/
	bool parse(const std::string& pText);

	bool append(const std::string& pText);
	void append(const text_format& pFormat);

	size_t get_block_count() const;
	const block& get_block(size_t pIndex) const;

	text_format& operator+=(const std::string& pText);
	text_format& operator+=(const text_format& pFormat);

	//void set_default_color();

	std::vector<block>::const_iterator begin() const;
	std::vector<block>::const_iterator end() const;

	text_format substr(size_t pOffset, size_t pCount = 0) const;

	bool word_wrap(size_t pLength);

	void remove_first_line();
	void limit_lines(size_t pLines);
	size_t line_count() const;
	size_t length() const;

private:
	bool append_parse(const std::string & pText);

	std::vector<block> mBlocks;
	color mDefault_color;
};

class formatted_text_node :
	public render_object
{
public:
	formatted_text_node();

	void set_font(std::shared_ptr<font> pFont, bool pApply_preferences = false);
	
	void set_text(const text_format& pText);
	const text_format& get_text() const;

	void set_color(const color& pColor);
	void set_anchor(anchor pAnchor);

	fvector get_size() const;

	void set_character_size(size_t pSize);

	virtual frect get_render_rect() const;

	virtual int draw(renderer &pR);

private:
	// Used for timing the effects
	float mTimer;

	std::shared_ptr<font> mFont;
	text_format mFormat;

	anchor mAnchor;

	fvector mSize;

	struct block_handle
	{
		fvector mOriginal_position;
		vertex_reference mVertices;
		size_t mBlock_index;
	};
	size_t mCharacter_size;
	std::vector<block_handle> mBlock_handles;
	vertex_batch mVertex_batch;
	void update_effects();
	void update();
};

class animation_node :
	public sprite_node
{
public:
	animation_node();

	size_t get_frame() const;
	void set_frame(frame_t pFrame);
	void set_animation(animation::ptr pAnimation, bool pSwap = false);
	bool set_animation(const std::string& pName, bool pSwap = false);

	bool tick(); // Returns true if the frame has changed.

	bool is_playing() const;
	void start();
	void pause();
	void stop();
	void restart();

	virtual int draw(renderer &r);

	float get_speed() const;
	void set_speed(float pSpeed);

private:
	engine::clock  mClock;

	animation::ptr mAnimation;

	frame_t mFrame;

	anchor mAnchor;

	float mInterval;
	float mSpeed;
	bool mPlaying;

	void update_frame();
};


}

#endif