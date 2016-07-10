#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "utility.hpp"

using namespace engine;

animated_sprite_node::animated_sprite_node()
{
	c_frame = 0;
	interval = 0;
	playing = false;
	loop = LOOP_LINEAR;
}

int 
animated_sprite_node::set_texture(texture& tex)
{
	_sprite.setTexture(tex.sfml_get_texture());
	return 0;
}

int
animated_sprite_node::add_frame(std::string name, texture& tex, std::string atlas)
{
	texture_crop crop;
	if (!tex.find_atlas(name, crop))
	{
		frames.push_back(crop);
	}
	return 0;
}

int 
animated_sprite_node::generate_sequence(int c, int width, int height, fvector offset)
{
	for (int i = 0; i < c; i++)
	{
		texture_crop crop;
		crop.x = i*width + (int)offset.x;
		crop.y = (int)offset.y;
		crop.w = width;
		crop.h = height;
		frames.push_back(crop);
	}
	return 0;
}

int 
animated_sprite_node::generate_sequence(int c, texture &tex, std::string atlas)
{
	texture_crop crop;
	if (!tex.find_atlas(atlas, crop))
		generate_sequence(c, crop.w, crop.h, { (float)crop.x, (float)crop.y });
	return 0;
}

void
animated_sprite_node::set_seq_interval()
{
	int h = 0;
	for (auto &i : seq_interval)
	{
		if (c_frame >= i.from_frame)
			h = i.interval;
	}
	interval = h;
}

void
animated_sprite_node::add_sequence_interval(int i, size_t from)
{
	seq_interval_entry ne;
	ne.interval = i;
	ne.from_frame = from;
	seq_interval.push_back(ne);

	// Set first frame interval
	if (from == 0) interval = i;
}

void
animated_sprite_node::tick_animation()
{
	int time = c_clock.get_elapse().ms_i();
	if (time >= interval && interval > 0)
	{
		c_clock.restart();

		// Calculate the next frame
		c_frame += time / interval;

		if (seq_interval.size())
			set_seq_interval();

		if (!loop && c_frame >= frames.size() - 1)
		{
			playing = false; // stop
			c_frame = frames.size() - 1;
		}
	}
}

engine::texture_crop&
animated_sprite_node::calculate_crop()
{
	if (loop == LOOP_LINEAR)
		return frames[c_frame%frames.size()];
	else if (loop == LOOP_PING_PONG)
		return frames[utility::pingpong_value(c_frame, frames.size())];
	return frames[c_frame];
}

int 
animated_sprite_node::draw(renderer &_r)
{
	if (!frames.size()) return 1;
	if (playing) tick_animation();
	texture_crop &crop = calculate_crop();
	_sprite.setTextureRect({ crop.x, crop.y, crop.w, crop.h });
	fvector loc = get_position();
	_sprite.setPosition(loc.x, loc.y);
	_r.window.draw(_sprite);
	return 0;
}

fvector
animated_sprite_node::get_size()
{
	if (frames.size() <= 0) return{ 0 };
	return{ (float)frames[0].w, (float)frames[0].h };
}

void
animated_sprite_node::set_anchor(anchor type)
{
	auto node_offset = engine::center_offset(get_size(), type);
	_sprite.setOrigin({ node_offset.x, node_offset.y });
}

void 
animated_sprite_node::set_interval(int _interval)
{
	interval = _interval;
}

void 
animated_sprite_node::start()
{
	playing = true;
	c_clock.restart();
}

void 
animated_sprite_node::pause()
{
	playing = false;
}

void 
animated_sprite_node::restart()
{
	c_frame = 0;
}

void
animated_sprite_node::stop()
{
	pause();
	restart();
}

bool
animated_sprite_node::is_playing()
{
	return playing;
}

void
animated_sprite_node::set_loop(int a)
{
	loop = a;
}