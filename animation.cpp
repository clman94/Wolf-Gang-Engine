#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "utility.hpp"

using namespace engine;


uni_animation::uni_animation()
{
	opt_texture = nullptr;
	default_frame = 0;
}

void
uni_animation::set_loop(int a)
{
	loop = a;
}

int
uni_animation::get_loop()
{
	return loop;
}

void
uni_animation::add_frame(engine::irect frame, int interval)
{
	if (get_interval() != interval)
		sequence.push_back({ interval, frames.size() });
	frames.push_back(frame);
}

void
uni_animation::add_interval(frame_t from, int interval)
{
	if (get_interval(from) != interval)
		sequence.push_back({ interval, from });
}

int
uni_animation::get_interval(frame_t at)
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
uni_animation::get_frame_count()
{
	return frames.size();
}

const engine::irect&
uni_animation::get_frame(frame_t frame)
{
	return frames.at(frame);
}

ivector
uni_animation::get_size()
{
	if (!frames.size())
		return 0;
	return frames[0].get_size();
}

void
uni_animation::set_default_frame(frame_t frame)
{
	default_frame = frame;
}

int
uni_animation::get_default_frame()
{
	return default_frame;
}

void
uni_animation::set_texture(engine::texture& texture)
{
	opt_texture = &texture;
}

engine::texture*
uni_animation::get_texture()
{
	return opt_texture;
}

void
uni_animation::generate(frame_t frame_count, engine::irect first_frame, engine::ivector scan)
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

uni_container::uni_container()
{
	c_anchor = anchor::topleft;
}

frame_t
uni_container::calculate_frame()
{
	assert(animation != nullptr);
	if (animation->get_loop() == uni_animation::LOOP_LINEAR)
		return c_count%animation->get_frame_count();

	if (animation->get_loop() == uni_animation::LOOP_PING_PONG)
		return utility::pingpong_value(c_count, animation->get_frame_count() - 1);

	return c_count%animation->get_frame_count();
}

void
uni_container::set_frame(frame_t frame)
{
	c_count = frame;
	if (animation)
		c_frame = calculate_frame();
}

void
uni_container::set_animation(uni_animation& a)
{
	animation = &a;
	interval = a.get_interval();
	set_frame (a.get_default_frame());
	if(a.get_texture())
		set_texture(*a.get_texture());
}

void
uni_container::set_texture(texture& tex)
{
	sfml_sprite.setTexture(tex.sfml_get_texture());
}

int
uni_container::tick()
{
	if (!animation) return 0;

	int time = clock.get_elapse().ms_i();
	if (time >= interval && interval > 0)
	{
		c_count += time / interval;
		c_frame = calculate_frame();

		interval = animation->get_interval(c_frame);
	}
	return 0;
}

bool
uni_container::is_playing()
{
	return playing;
}

void
uni_container::start()
{
	clock.restart();
	playing = true;
}

void
uni_container::pause()
{
	clock.pause();
	playing = false;
}

void
uni_container::stop()
{
	playing = false;
	restart();
}

void
uni_container::restart()
{
	if (animation)
		set_frame(animation->get_default_frame());
}

void
uni_container::set_anchor(engine::anchor a)
{
	c_anchor = a;
}

int
uni_container::draw(renderer &_r)
{
	if (!animation) return 1;
	if (playing) tick();
	const engine::irect &crop = animation->get_frame(c_frame);
	sfml_sprite.setTextureRect({ crop.x, crop.y, crop.w, crop.h });
	fvector loc = get_position() + anchor_offset(animation->get_size(), c_anchor);
	sfml_sprite.setPosition(loc.x, loc.y);
	_r.window.draw(sfml_sprite);
	return 0;
}
