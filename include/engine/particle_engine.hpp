#ifndef ENGINE_PARTICLE_SYSTEM
#define ENGINE_PARTICLE_SYSTEM

#include "renderer.hpp"
#include "time.hpp"
#include <vector>
#include "utility.hpp"
#include "node.hpp"

namespace engine
{

// TODO: Add different types of emitter sources.
class particle_emitter :
	public render_object
{
public:
	particle_emitter();
	void spawn(size_t count = 1);
	void set_region(fvector size);
	void set_life(float a);
	void set_acceleration(fvector a);
	void set_velocity(fvector a);
	void set_rate(float a);
	void set_texture(std::shared_ptr<texture> pTexture);
	void set_texture_rect(frect r);
	int draw(renderer &_r);

private:
	elapse_clock mFrame_clock, mSpawn_clock;

	struct particle
	{
		timer life;
		fvector velocity;
		bool valid;
		engine::vertex_reference sprite;
	};
	std::vector<particle> mParticles;

	particle* find_unused_particle();

	fvector mRegion_size;
	float   mRate, mLife;
	fvector mAcceleration;
	fvector mVelocity;

	vertex_batch  mSprites;
	frect         mTexture_rect;

	void tick();
};


}

#endif