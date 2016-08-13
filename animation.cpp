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
animation::set_loop(e_loop a)
{
	loop = a;
}

animation::e_loop
animation::get_loop()
{
	return loop;
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

void engine::animation::set_frame_count(frame_t count)
{
	frame_count = count;
}

frame_t
animation::get_frame_count()
{
	return frame_count;
}

void animation::set_frame_rect(engine::frect rect)
{
	frame = rect;
}

engine::frect
animation::get_frame_at(frame_t at)
{
	engine::frect ret = frame;
	ret.x += ret.w*(at%frame_count);
	return ret;
}

fvector
animation::get_size()
{
	return frame.get_size();
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

animation_node::animation_node()
{
	c_anchor = anchor::topleft;
	playing = false;
	c_animation = nullptr;
	add_child(sprite);
}

frame_t
animation_node::calculate_frame()
{
	assert(c_animation != nullptr);
	if (c_animation->get_frame_count() == 0)
		return 0;

	if (c_animation->get_loop() == animation::e_loop::linear)
		return c_count%c_animation->get_frame_count();

	else if (c_animation->get_loop() == animation::e_loop::pingpong)
		return util::pingpong_index(c_count, c_animation->get_frame_count() - 1);

	else
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
animation_node::set_animation(animation& a, bool swap)
{
	c_animation = &a;
	interval = a.get_interval();

	if (swap) c_frame = calculate_frame();
	else set_frame(a.get_default_frame());

	if (a.get_texture())
		set_texture(*a.get_texture());
}

void
animation_node::set_texture(texture& tex)
{
	sprite.set_texture(tex);
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

		clock.restart();
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
	sprite.set_anchor(a);
}

int
animation_node::draw(renderer &r)
{
	if (!c_animation) return 1;
	if (!c_animation->get_frame_count()) return 1;
	if (playing) tick();

	frect rect = c_animation->get_frame_at(c_frame);
	sprite.set_texture_rect(rect);
	sprite.set_anchor(c_anchor);

	sprite.draw(r);
	return 0;
}
