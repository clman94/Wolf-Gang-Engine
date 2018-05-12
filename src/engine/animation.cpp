#define ENGINE_INTERNAL

#include <engine/renderer.hpp>
#include <engine/utility.hpp>
#include <engine/logger.hpp>
#include <engine/math.hpp>

using namespace engine;


animation::animation()
{
	mDefault_frame = 0;
	mFrame_count = 1;
	mLoop = loop_type::linear;
}

void animation::set_loop(loop_type pLoop)
{
	mLoop = pLoop;
}

animation::loop_type animation::get_loop() const
{
	return mLoop;
}

void animation::add_interval(frame_t pFrom, float pInterval)
{
	if (get_interval(pFrom) != pInterval)
		mSequence.push_back({ pInterval, pFrom });
}

float animation::get_interval(frame_t pAt) const
{
	float retval = 0;
	frame_t last = 0;
	for (auto& i : mSequence)
	{
		if (i.from <= pAt && i.from >= last)
		{
			retval = i.interval;
			last = i.from;
		}
	}
	return retval;
}

void animation::set_frame_count(frame_t pCount)
{
	assert(pCount >= 1);
	mFrame_count = pCount;
}

frame_t animation::get_frame_count() const
{
	return mFrame_count;
}

void animation::set_frame_rect(engine::frect pRect)
{
	mFrame_rect = pRect;
}

engine::frect animation::get_frame_at(frame_t pAt) const
{
	if (mFrame_count == 1)
		return mFrame_rect;
	engine::frect ret = mFrame_rect;
	ret.x += ret.w*calc_frame_from_count(pAt);
	return ret;
}

frect animation::get_frame_at_time(float pSeconds) const
{
	return get_frame_at(calc_frame_from_time(pSeconds));
}

frect animation::get_root_frame() const
{
	return mFrame_rect;
}

fvector animation::get_size() const
{
	return mFrame_rect.get_size();
}

void animation::set_default_frame(frame_t pFrame)
{
	mDefault_frame = pFrame;
}

frame_t animation::get_default_frame() const
{
	return mDefault_frame;
}

frect animation::get_full_rect() const
{
	frect retval(mFrame_rect);
	retval.w *= mFrame_count;
	return retval;
}

frame_t animation::calc_frame_from_count(frame_t pCount) const
{
	if (mFrame_count <= 1)
		return 0;
	switch (mLoop)
	{
	case animation::loop_type::none:
		return util::clamp(pCount, 0, mFrame_count - 1);

	case animation::loop_type::linear:
		return math::pmod(pCount, mFrame_count);

	case animation::loop_type::pingpong:
		frame_t end = mFrame_count - 1;
		return (pCount / end) % 2 ? end - math::pmod(pCount, end) : math::pmod(pCount, end);
	}
	return 0;
}

frame_t animation::calc_frame_from_time(float pSeconds) const
{
	if (mFrame_count <= 1 || get_interval() == 0)
		return 0;
	return calc_frame_from_count(static_cast<int>(pSeconds / get_interval()) + mDefault_frame);
}

animation_node::animation_node()
{
	mAnchor = anchor::topleft;
	mAnimation = nullptr;
	mSpeed = 1.f;
	mInterval = 1;
	mFrame = 0;
}

frame_t animation_node::get_frame() const
{
	if (mAnimation->get_frame_count() == 1)
		return 0;
	return mAnimation->calc_frame_from_count(mFrame);
}

void animation_node::set_frame(frame_t pFrame)
{
	mFrame = pFrame;
	mClock.restart();
	mInterval = mAnimation->get_interval(mFrame);
	update_frame();
}

void animation_node::set_animation(animation::ptr pAnimation, bool pSwap)
{
	mAnimation = pAnimation;
	mInterval = pAnimation->get_interval();

	if (!pSwap)
		set_frame(pAnimation->get_default_frame());
	else
		update_frame();

	//if (pAnimation.get_texture())
	//	set_texture(*pAnimation.get_texture());
}

bool animation_node::set_animation(const std::string& pName, bool pSwap)
{
	auto texture = get_texture();
	if (!texture)
		return false;

	auto entry = texture->get_entry(pName);
	if (!entry)
		return false;
	set_animation(entry, pSwap);
	return true;
}

bool animation_node::tick()
{
	if (!mAnimation) return false;
	if (mInterval <= 0) return false;

	const float time = mClock.get_elapse().milliseconds();
	const float scaled_interval = mInterval/mSpeed;

	if (time >= scaled_interval)
	{
		mFrame += static_cast<frame_t>(std::floor(time / scaled_interval));

		mInterval = mAnimation->get_interval(mFrame);

		if (mFrame > mAnimation->get_frame_count()
			&& mAnimation->get_loop() == animation::loop_type::none)
			pause();

		mClock.restart();

		update_frame();
		return true;
	}
	return false;
}

bool animation_node::is_playing() const
{
	return !mClock.is_paused();
}

void animation_node::start()
{
	// Restart animation if reached end
	if (mAnimation)
		if (mAnimation->get_loop() == engine::animation::loop_type::none
			&&  mFrame >= mAnimation->get_frame_count())
			restart();

	if (!is_playing())
	{
		mClock.restart();
		mClock.resume();
	}
}

void animation_node::pause()
{
	mClock.pause();
}

void animation_node::stop()
{
	restart();
}

void animation_node::restart()
{
	if (mAnimation)
	{
		set_frame(mAnimation->get_default_frame());
		mClock.restart();
	}
}

int animation_node::draw(renderer &pR)
{
	if (!mAnimation) return 1;
	if (is_playing())
		tick();
	sprite_node::draw(pR);
	return 0;
}

float animation_node::get_speed() const
{
	return mSpeed;
}

void animation_node::set_speed(float pSpeed)
{
	if (pSpeed <= 0)
	{
		logger::error("Animation speed cannot be less than or equal to 0");
		return;
	}
	mSpeed = pSpeed;
}

void animation_node::update_frame()
{
	if (mAnimation)
		set_texture_rect(mAnimation->get_frame_at(mFrame));
}
