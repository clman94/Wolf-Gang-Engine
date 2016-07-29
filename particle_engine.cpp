#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "particle_engine.hpp"
#include <cmath>

using namespace engine;


void
particle_system::spawn_particle(size_t count)
{
	while (count-- != 0)
	{
		particles.emplace_back();
		auto &p = particles.back();
		p.timer.restart();
		p.velocity = emitter.velocity;
		p.pos = {
			std::fmodf((float)std::rand(), emitter.size.x),
			std::fmodf((float)std::rand(), emitter.size.y)
		};
		p.sprite = sprites.add_sprite(p.pos, texture_rect);
	}
}

void
particle_system::step()
{
	if (emitter.rate > 0 && spawnclock.get_elapse().s() >= emitter.rate)
	{
		spawn_particle();
		spawnclock.restart();
	}

	time_t time = frameclock.get_elapse().s();
	for (auto &i : particles)
	{
		i.velocity += emitter.acceleration*time;
		i.pos += i.velocity*time;
		sprites.set_sprite_position(i.sprite, i.pos, texture_rect);
	}

	frameclock.restart();
}

void
particle_system::set_region(fvector a)
{
	emitter.size = a;
}

void
particle_system::set_life(float a)
{
	emitter.life = a;
}

void
particle_system::set_acceleration(fvector a)
{
	emitter.acceleration = a;
}

void
particle_system::set_velocity(fvector a)
{
	emitter.velocity = a;
}

void
particle_system::set_rate(float a)
{
	emitter.rate = a;
}

void
particle_system::set_texture(texture &t)
{
	sprites.set_texture(t);
}

void
particle_system::set_texture_rect(frect r)
{
	texture_rect = r;
}

int
particle_system::draw(renderer &_r)
{
	step();
	sprites.set_position(get_exact_position());
	sprites.draw(_r);
	return 0;
}