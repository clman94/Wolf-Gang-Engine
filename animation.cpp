#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "utility.hpp"

using namespace engine;


animation::animation()
{
	opt_texture = nullptr;
	default_frame = 0;
}

void
animation::set_loop(int a)
{
	loop = a;
}

int
animation::get_loop()
{
	return loop;
}

void
animation::add_frame(engine::irect frame, int interval)
{
	if (get_interval() != interval)
		sequence.push_back({ interval, frames.size() });
	frames.push_back(frame);
}

void
animation::add_interval(frame_t from, int interval)
{
	if (get_interval(from) != interval)
		sequence.push_back({ interval, from });
}

int
animation::get_interval(frame_t at)
{
	int retval = 0;
	frame_t last = 0;
	for (auto& i : sequence)
	{
		if (i.from <= at && i.from >= last) 
		{
			retval = i.interval;
			last = i.from;
		}
	}
	return retval;
}

frame_t
animation::get_frame_count()
{
	return frames.size();
}

const engine::irect&
animation::get_frame(frame_t frame)
{
	return frames.at(frame);
}

ivector
animation::get_size()
{
	if (!frames.size())
		return 0;
	return frames[0].get_size();
}

void
animation::set_default_frame(frame_t frame)
{
	default_frame = frame;
}

int
animation::get_default_frame()
{
	return default_frame;
}

void
animation::set_texture(texture& _texture)
{
	opt_texture = &_texture;
}

engine::texture*
animation::get_texture()
{
	return opt_texture;
}

void
animation::generate(frame_t frame_count, irect first_frame, ivector scan)
{
	auto offset = first_frame.get_offset();
	auto size = first_frame.get_size();
	for (frame_t i = 0; i < frame_count; i++)
	{
		auto nframe = first_frame;
		nframe.set_offset((scan*size*i) + offset);
		frames.push_back(nframe);
	}
}

animation_node::animation_node()
{
	c_anchor = anchor::topleft;
}

frame_t
animation_node::calculate_frame()
{
	assert(c_animation != nullptr);
	if (c_animation->get_frame_count() == 0)
		return 0;

	if (c_animation->get_loop() == animation::LOOP_LINEAR)
		return c_count%c_animation->get_frame_count();

	if (c_animation->get_loop() == animation::LOOP_PING_PONG)
		return util::pingpong_index(c_count, c_animation->get_frame_count() - 1);

	return c_count%c_animation->get_frame_count();
}

void
animation_node::set_frame(frame_t frame)
{
	c_count = frame;
	if (c_animation)
		c_frame = calculate_frame();
}

void
animation_node::set_animation(animation& a)
{
	c_animation = &a;
	interval = a.get_interval();
	set_frame (a.get_default_frame());
	if(a.get_texture())
		set_texture(*a.get_texture());
}

void
animation_node::set_texture(texture& tex)
{
	sfml_sprite.setTexture(tex.sfml_get_texture());
}

int
animation_node::tick()
{
	if (!c_animation) return 0;

	int time = clock.get_elapse().ms_i();
	if (time >= interval && interval > 0)
	{
		c_count += time / interval;
		c_frame = calculate_frame();

		interval = c_animation->get_interval(c_frame);
	}
	return 0;
}

bool
animation_node::is_playing()
{
	return playing;
}

void
animation_node::start()
{
	clock.restart();
	playing = true;
}

void
animation_node::pause()
{
	clock.pause();
	playing = false;
}

void
animation_node::stop()
{
	playing = false;
	restart();
}

void
animation_node::restart()
{
	if (c_animation)
		set_frame(c_animation->get_default_frame());
}

void
animation_node::set_anchor(engine::anchor a)
{
	c_anchor = a;
}

int
animation_node::draw(renderer &_r)
{
	if (!c_animation) return 1;
	if (!c_animation->get_frame_count()) return 1;
	if (playing) tick();
	const engine::irect &crop = c_animation->get_frame(c_frame);
	sfml_sprite.setTextureRect({ crop.x, crop.y, crop.w, crop.h });
	fvector loc = get_position() + anchor_offset(c_animation->get_size(), c_anchor);
	sfml_sprite.setPosition(loc.x, loc.y);
	_r.window.draw(sfml_sprite);
	return 0;
}
