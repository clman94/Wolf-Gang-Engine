#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "particle_engine.hpp"
#include <cmath>

using namespace engine;

void particle_system::spawn_particle(size_t pCount)
{
	for (size_t i = 0; i < pCount; i++)
	{
		auto nparticle = find_unused_particle();
		if (!nparticle) {
			mParticles.emplace_back();
			nparticle = &mParticles.back();
		}
		nparticle->life.start_timer(mEmitter.life);
		nparticle->velocity = mEmitter.velocity;
		nparticle->valid = true;

		fvector position = {
			std::fmodf((float)std::rand(), mEmitter.size.x),
			std::fmodf((float)std::rand(), mEmitter.size.y)
		};
		nparticle->sprite = mSprites.add_quad(position, mTexture_rect);
	}
}

particle_system::particle * particle_system::find_unused_particle()
{
	for (auto &i : mParticles)
	{
		if (!i.valid)
			return &i;
	}
	return nullptr;
}

particle_system::particle_system()
{
	add_child(mSprites);
}

void
particle_system::tick()
{
	if (mEmitter.rate > 0 && mSpawn_clock.get_elapse().s() >= mEmitter.rate)
	{
		spawn_particle();
		mSpawn_clock.restart();
	}

	time_t time = mFrame_clock.get_elapse().s();
	for (auto &i : mParticles)
	{
		if (!i.valid)
			continue;

		if (i.life.is_reached())
		{
			i.sprite.hide();
			i.valid = false;
		}
		else {
			i.velocity += mEmitter.acceleration*time;
			auto position = i.sprite.get_position();
			i.sprite.set_position(position + i.velocity*time);
		}
	}

	mFrame_clock.restart();
}

void
particle_system::set_region(fvector a)
{
	mEmitter.size = a;
}

void
particle_system::set_life(float a)
{
	mEmitter.life = a;
}

void
particle_system::set_acceleration(fvector a)
{
	mEmitter.acceleration = a;
}

void
particle_system::set_velocity(fvector a)
{
	mEmitter.velocity = a;
}

void
particle_system::set_rate(float a)
{
	mEmitter.rate = a;
}

void
particle_system::set_texture(texture &t)
{
	mSprites.set_texture(t);
}

void
particle_system::set_texture_rect(frect r)
{
	mTexture_rect = r;
}

int
particle_system::draw(renderer &_r)
{
	tick();
	mSprites.draw(_r);
	return 0;
}