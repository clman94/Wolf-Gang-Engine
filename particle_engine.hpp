#ifndef ENGINE_PARTICLE_SYSTEM
#define ENGINE_PARTICLE_SYSTEM

#include "renderer.hpp"
#include "time.hpp"
#include <vector>
#include "utility.hpp"
#include "node.hpp"

namespace engine
{
class particle_system :
	public render_client,
	public node
{
	clock frameclock, spawnclock;

	struct particle
	{
		clock   timer;
		fvector velocity;
		engine::vertex_reference  sprite;
	};
	std::vector<particle> particles;

	struct
	{
		fvector size;
		float   rate, life;
		fvector acceleration;
		fvector velocity;
	} emitter;

	vertex_batch  sprites;
	frect         texture_rect;

public:
	particle_system();
	void step();
	void spawn_particle(size_t count = 1);
	void set_region(fvector size);
	void set_life(float a);
	void set_acceleration(fvector a);
	void set_velocity(fvector a);
	void set_rate(float a);
	void set_texture(texture &t);
	void set_texture_rect(frect r);
	int draw(renderer &_r);
};

}

#endif