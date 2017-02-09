#define ENGINE_INTERNAL

#include <engine/renderer.hpp>
#include <engine/particle_engine.hpp>

#include <cmath>

using namespace engine;

void particle_emitter::spawn(size_t pCount)
{
	for (size_t i = 0; i < pCount; i++)
	{
		auto nparticle = find_unused_particle();
		if (!nparticle) {
			mParticles.emplace_back();
			nparticle = &mParticles.back();
		}
		nparticle->life.start(mLife);
		nparticle->velocity = mVelocity;
		nparticle->valid = true;

		fvector position = {
			std::fmod((float)std::rand(), mRegion_size.x),
			std::fmod((float)std::rand(), mRegion_size.y)
		};
		nparticle->sprite = mSprites.add_quad(position, mTexture_rect);
	}
}

particle_emitter::particle * particle_emitter::find_unused_particle()
{
	for (auto &i : mParticles)
	{
		if (!i.valid)
			return &i;
	}
	return nullptr;
}

particle_emitter::particle_emitter()
{
	add_child(mSprites);
}

void
particle_emitter::tick()
{
	if (mRate > 0 && mSpawn_clock.get_elapse().s() >= mRate)
	{
		spawn();
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
			i.velocity += mAcceleration*time;
			auto position = i.sprite.get_position();
			i.sprite.set_position(position + i.velocity*time);
		}
	}

	mFrame_clock.restart();
}

void
particle_emitter::set_region(fvector a)
{
	mRegion_size = a;
}

void
particle_emitter::set_life(float a)
{
	mLife = a;
}

void
particle_emitter::set_acceleration(fvector a)
{
	mAcceleration = a;
}

void
particle_emitter::set_velocity(fvector a)
{
	mVelocity = a;
}

void
particle_emitter::set_rate(float a)
{
	mRate = a;
}

void
particle_emitter::set_texture(std::shared_ptr<texture> pTexture)
{
	mSprites.set_texture(pTexture);
}

void
particle_emitter::set_texture_rect(frect r)
{
	mTexture_rect = r;
}

int
particle_emitter::draw(renderer & pR)
{
	tick();
	mSprites.draw(pR);
	return 0;
}
