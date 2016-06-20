#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SFML\Graphics.hpp>

#include "node.hpp"
#include "texture.hpp"
#include "ptr_GC.hpp"
#include "time.hpp"

#include <memory>
#include <vector>
#include <string>
#include <stdint.h>
#include <list>
#include <map>
#include <unordered_map>

namespace engine
{

struct color
{
	uint8_t r, g, b, a;
	color(
		uint8_t _r = 0,
		uint8_t _g = 0,
		uint8_t _b = 0,
		uint8_t _a = 255)
		: r(_r), g(_g), b(_b), a(_a)
	{}
};

class render_client;

class renderer
{
	sf::RenderWindow window;
	std::vector<render_client*> clients;
	int draw_clients();
	
	std::unordered_map<int, bool> pressed_keys;
	void refresh_clients();
public:
	typedef sf::Keyboard::Key key_type;

	renderer();
	~renderer();
	int initualize(int _width, int _height, int fps = 30);
	int draw();
	int update_events();
	int close();
	int add_client(render_client* _client);
	int remove_client(render_client* _client);
	void set_pixel_scale(float a);
	bool is_key_pressed(key_type k);
	bool is_key_held(key_type k);
	fvector get_size();
	void sort_clients();

	friend class sprite_node;
	friend class tile_node;
	friend class animated_sprite_node;
	friend class text_node;
	friend class rectangle_node;
};

class render_client
{
	renderer* renderer_;
	int client_index;
	bool visible;
	int depth;
public:
	render_client();
	~render_client();
	void set_depth(int d);
	int get_depth();
	bool is_visible();
	void set_visible(bool a);
	virtual int draw(renderer &_r) = 0;
	int is_rendered();
	friend class renderer;
};

// Wraps a client. Allows the client to be swapped in runtime.
// Visibiliy of the wrapped client is ignored.
// Wrapped client does not need to be registered
// within the renderer.
class render_client_wrapper :
	public render_client
{
	render_client* ptr;
public:
	render_client_wrapper();
	int draw(renderer &_r);
	void bind_client(render_client* client);
	render_client* get_client();
};

template<typename T>
class render_client_special :
	public render_client
{
	T* ptr;
public:
	render_client_special()
	{
		ptr = nullptr;
	}
	int draw(renderer &_r)
	{
		if (!ptr) return 0;
		return ptr->draw(_r);
	}
	void bind_client(T* client)
	{
		ptr = client;
	}
	T* get_client()
	{
		return ptr;
	}

	T& operator->()
	{
		return *ptr;
	}
};

enum anchor
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

class rectangle_node :
	public render_client,
	public node
{
	sf::RectangleShape shape;
public:
	void set_color(color c)
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
		shape.setSize({ s.x, s.y });
	}
	virtual int draw(renderer &_r)
	{
		fvector loc = get_position();
		shape.setPosition(sf::Vector2f((float)loc.x, (float)loc.y));
		_r.window.draw(shape);
		return 0;
	}
};

class sprite_node : 
	public render_client, 
	public node
{
	sf::Sprite _sprite;
	sf::Vector2f offset;
public:
	void set_anchor(anchor _anchor);
	virtual int draw(renderer &_r);
	void set_scale(fvector s);
	int set_texture(texture& tex);
	int set_texture(texture& tex, std::string atlas);
	fvector get_size();
};

class animated_sprite_node :
	public render_client,
	public node
{
	sf::Sprite _sprite;
	bool play;
	engine::clock c_clock;
	std::vector<texture_crop> frames;
	int interval;
	size_t c_frame;
	bool playing, loop;
public:
	animated_sprite_node();
	virtual int draw(renderer &_r);
	int set_texture(texture& tex);
	int add_frame(std::string name, texture& tex, std::string atlas);
	int generate_sequence(int frames, int width, int height, fvector offset = { 0 });
	int generate_sequence(int frames, texture& tex, std::string atlas);
	fvector get_size();
	void set_interval(int _interval);
	void tick_animation();
	void start();
	void pause();
	void stop();
	void restart();
	void set_frame();
	void set_default_frame();
	void set_loop(bool a);
	bool is_playing();
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
public:
	void set_font(font& f);
	void set_text(const std::string s);
	void append_text(const std::string s);
	std::string get_text();
	void set_size(int s);
	void set_color(int r, int g, int b);
	void set_scale(float a);
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
		ivector pos;
		size_t index, layer;
		tile_entry(ivector _pos, size_t _index, size_t _layer)
			: pos(_pos), index(_index), layer(_layer){}
	};
	std::vector<tile_entry> entries;
	int find_tile(ivector pos);
	ivector tile_size;
	std::map<size_t, ptr_GC_owner<std::vector<sf::Vertex>>> layers;
	texture* c_tex;
	size_t c_layer;
public:
	tile_node();
	void set_layer(size_t layer);
	void set_tile_size(ivector s);
	void set_texture(texture& tex);
	void set_tile(ivector pos, std::string atlas, int rot = 0, bool replace = true);
	void clear_all();
	virtual int draw(renderer &_r);
};

}

#endif