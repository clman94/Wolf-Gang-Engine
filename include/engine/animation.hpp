#ifndef ENGINE_ANIMATION_HPP
#define ENGINE_ANIMATION_HPP

#include <cassert>
#include <vector>

#include "texture.hpp"
#include "resource.hpp"
#include "types.hpp"
#include "rect.hpp"

namespace engine
{

class texture;

class animation
{
public:
	animation();

	enum class loop_type
	{
		none,
		linear,
		pingpong
	};

	void set_loop(loop_type pLoop);
	loop_type  get_loop() const;

	void add_interval(frame_t pFrom, float pInterval);

	float get_interval(frame_t pAt = 0) const;

	void set_frame_count(frame_t pCount);
	frame_t get_frame_count() const;

	void set_frame_rect(frect pRect) ;
	frect get_frame_at(frame_t pAt) const;

	fvector get_size() const;

	void set_default_frame(frame_t pFrame);
	frame_t  get_default_frame() const;

	frect full_region() const;

private:
	struct sequence_frame
	{
		float   interval;
		frame_t from;
	};
	std::vector<sequence_frame> mSequence;
	frect               mFrame_rect;
	frame_t                     mDefault_frame;
	frame_t                     mFrame_count;
	loop_type                   mLoop;
	frame_t calculate_frame(frame_t pCount) const;
};


}

#endif