#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SFML\Graphics.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <list>
#include <map>
#include <unordered_map>
#include <cassert>

#include "node.hpp"
#include "texture.hpp"
#include "ptr_GC.hpp"
#include "time.hpp"
#include "rect.hpp"
#include "types.hpp"
#include "utility.hpp"

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
};

class render_client;

class events
{
	sf::RenderWindow *window;

	sf::Event event;

	std::map<int, int> pressed_keys;
	std::map<int, int> pressed_buttons;
	void refresh_pressed();

public:
	typedef sf::Keyboard::Key key_type;

	enum mouse_button
	{
		mouse_left,
		mouse_right,
		mouse_middle
	};

	bool is_key_pressed(key_type k);
	bool is_key_down(key_type k);
	bool is_mouse_pressed(mouse_button b);
	bool is_mouse_down(mouse_button b);

	int update_events();

protected:
	void events_update_sfml_window(sf::RenderWindow& window);
};

class renderer :
	public events,
	public util::nocopy
{
	sf::RenderWindow window;
	std::vector<render_client*> clients;
	int draw_clients();

	void refresh_clients();

	struct{
		bool enable;
		std::string text;
		std::string* ptr;
		bool multi_line;
	} text_record;

	color background_color;

public:

	renderer();
	~renderer();
	int initualize(ivector size, int fps = 30);
	int draw();
	
	int close();
	int add_client(render_client* _client);
	int remove_client(render_client* _client);
	void set_pixel_scale(float a);

	fvector get_size();
	void sort_clients();

	void start_text_record(bool multi_line = false);
	void start_text_record(std::string& ptr, bool multi_line = false);
	bool is_text_recording();
	bool is_text_recording(std::string& ptr);
	void end_text_record();

	const std::string& get_recorded_text();

	fvector get_mouse_position();
	fvector get_mouse_position(fvector relative);

	bool is_focused();

	void set_visible(bool is_visible);
	void set_bg_color(color c);

#ifdef ENGINE_INTERNAL

	sf::RenderWindow& get_sfml_window()
	{ return window; }

#endif

	friend class sprite_node;
	friend class tile_node;
	friend class text_node;
	friend class rectangle_node;
};

class render_client :
	public util::nocopy
{
	renderer* renderer_;
	int client_index;
	bool visible;
	depth_t depth;
public:
	render_client();
	~render_client();
	void set_depth(depth_t d);
	float get_depth();
	bool is_visible();
	void set_visible(bool a);
	virtual int draw(renderer &_r) = 0;
	int is_rendered();
	friend class renderer;

protected:
	virtual void refresh_renderer(renderer& _r) {}
};

class render_proxy
{
	renderer* r;
public:
	void set_renderer(renderer& _r)
	{
		r = &_r;
		refresh_renderer(_r);
	}
	renderer* get_renderer()
	{
		assert(r != nullptr);
		return r;
	}
protected:
	virtual void refresh_renderer(renderer& _r){}
};

class vertex_reference
{
public:
	vertex_reference()
		: ref(nullptr)
	{}

#ifdef ENGINE_INTERNAL

	vertex_reference(sf::Vertex& _ref)
		: ref(&_ref)
	{}

	vertex_reference& operator=(sf::Vertex& _ref)
	{
		ref = &_ref;
		return *this;
	}

#endif

	void set_position(fvector position);
	fvector get_position();
	void set_texture_rect(frect rect, int rotation);
private:
	sf::Vertex* ref;
	void refresh_size();
};

class vertex_batch :
	public render_client,
	public node
{
public:
	void set_texture(texture &t);
	vertex_reference add_sprite(fvector pos, frect tex_rect, int rot = 0);
	int draw(renderer &_r);

private:
	std::vector<sf::Vertex> vertices;
	texture *c_texture;
};

enum struct anchor
{
	top,
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
static vector<T> center_offset(const vector<T>& size, anchor type)
{
	switch (type)
	{
	case anchor::top:
		return{ size.x/2, 0 };
	case anchor::topleft:
		return{ 0, 0 };
	case anchor::topright:
		return{ size.x, 0 };
	case anchor::bottom:
		return{ size.x/2, size.y };
	case anchor::bottomleft:
		return{ 0, size.y };
	case anchor::bottomright:
		return{ size.x, size.y };
	case anchor::left:
		return{ 0, size.y/2 };
	case anchor::right:
		return{ size.x, size.y/2 };
	case anchor::center:
		return{ size.x/2, size.y/2 };
	}
	return 0;
}

template<typename T>
static vector<T> anchor_offset(const vector<T> size, anchor type)
{
	return center_offset(size, type) * -1;
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
	virtual int draw(renderer &_r)
	{
		auto pos = get_exact_position();
		shape.setPosition({pos.x, pos.y});
		_r.window.draw(shape);
		return 0;
	}
};

class sprite_node : 
	public render_client, 
	public node
{
	sf::Sprite _sprite;
	fvector offset;

public:
	void set_anchor(anchor _anchor);
	virtual int draw(renderer &_r);
	void set_scale(fvector scale);
	int set_texture(texture& tex);
	int set_texture(texture& tex, std::string atlas);
	void set_texture_rect(const engine::irect& crop);
	fvector get_size();
};

class font
{
	sf::Font sf_font;
public:
	int load(std::string path);
	friend class text_node;
};

class text_node :
	public render_client,
	public node
{
	sf::Text text;
	std::string c_text; // Avoids reliance on sfml
	engine::anchor c_anchor;
public:
	text_node();
	void set_font(font& f);
	void set_text(const std::string s);
	void append_text(const std::string s);
	std::string get_text();
	void set_character_size(int s);
	void set_anchor(engine::anchor a);
	void set_color(const color c);
	void set_scale(float a);
	void copy_format(const text_node& node);
	virtual int draw(renderer &_r);
};

class tile_sprite_node :
	public node
{
	struct tile_sprite
	{
		ivector pos;
		ptr_GC<sprite_node> sprite;
		tile_sprite(){}
		tile_sprite(ivector _pos, ptr_GC<sprite_node> _sprite)
			: pos(_pos), sprite(_sprite){}
	};
	std::vector<tile_sprite> tiles;
	float width, height;
	tile_sprite* find_tile(int x, int y);
	tile_sprite* find_tile(int x, int y, size_t& index);
	tile_sprite* create_tile(int x, int y);
public:
	void set_tile_size(float w, float h);
	int set_tile(int x, int y, texture& tex);
	int set_tile(int x, int y, texture& tex, std::string atlas);
	int remove_tile(int x, int y);
	int update_tile_render(renderer &_r);
	ptr_GC<sprite_node> get_sprite(int x, int y);
};

class tile_bind_node :
	public node
{
	struct tile_entry
	{
		ivector pos;
		ptr_GC<node> node;
		tile_entry(ivector _pos, ptr_GC<engine::node>& _node)
			: pos(_pos), node(_node){}
	};
	std::vector<tile_entry> tiles;
	size_t find_tile(ivector pos);
	ivector tile_size;
public:
	void set_tile_size(ivector s);
	int bind_tile(ptr_GC<node> n, ivector pos, bool replace = true);
	ptr_GC<node> unbind_tile(ivector pos);
	void clear_all();
};

class tile_node :
	public render_client,
	public node
{
	struct tile_entry{
		fvector pos;
		size_t index, layer;
		tile_entry(fvector _pos, size_t _index, size_t _layer)
			: pos(_pos), index(_index), layer(_layer){}
	};
	std::vector<tile_entry> entries;
	int find_tile(fvector pos, size_t layer = 0);
	fvector tile_size;
	std::map<size_t, ptr_GC_owner<std::vector<sf::Vertex>>> layers;
	texture* c_tex;
public:
	tile_node();
	void set_tile_size(fvector s);
	void set_texture(texture& tex);
	void set_tile(fvector pos, std::string atlas, size_t layer = 0, int rot = 0, bool replace = true);
	void remove_tile(fvector pos, size_t layer);
	void clear_all();
	virtual int draw(renderer &_r);
};


class animation
{
public:
	animation();

	enum class e_loop
	{
		none,
		linear,
		pingpong
	};

	void set_loop(e_loop a);
	e_loop  get_loop();

	void add_interval(frame_t from, int interval);

	int  get_interval(frame_t at = 0);

	void set_frame_count(frame_t count);
	frame_t get_frame_count();

	void set_frame_rect(engine::irect rect);
	engine::irect get_frame_at(frame_t at);

	ivector get_size();

	void set_default_frame(frame_t frame);
	int  get_default_frame();

	void set_texture(engine::texture& texture);
	engine::texture* get_texture();

private:
	struct sequence_frame
	{
		int     interval;
		frame_t from;
	};
	std::vector<sequence_frame> sequence;
	engine::irect               frame;
	engine::texture*            opt_texture;
	frame_t                     default_frame;
	frame_t                     frame_count;
	e_loop                      loop;
};

class animation_node :
	public render_client,
	public node
{
public:
	animation_node();

	void set_frame(frame_t frame);
	void set_animation(animation& a, bool swap = false);
	void set_texture(texture& tex);

	int tick();

	bool is_playing();
	void start();
	void pause();
	void stop();
	void restart();

	void set_anchor(anchor a);

	int draw(renderer &r);

private:
	sprite_node sprite;

	engine::clock  clock;

	animation* c_animation;

	frame_t c_count;
	frame_t c_frame;

	anchor c_anchor;

	int interval;
	bool playing;

	frame_t calculate_frame();
};

class tile_animation
{
public:
	void set_frame_rect(frect rect)
	{ frame_rect = rect; }

	void set_frame_count(frame_t count)
	{ frame_count = count; }

private:
	frect frame_rect;
	frame_t frame_count;
};


class tilemap_node_remake
{
public:
	void set_tile_size(fvector size)
	{
		tile_size = size;
	}



private:

	struct tile_entry
	{
		fvector position;
		size_t layer, index;
		frame_t frame;
		tile_animation* anim;
		tile_entry(fvector _pos, size_t _index, size_t _layer)
			: position(_pos), index(_index), layer(_layer) {}
	};

	std::vector<tile_entry> entries;
	std::map<size_t, ptr_GC_owner<std::vector<sf::Vertex>>> layers;
	fvector tile_size;
};


}

#endif