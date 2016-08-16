#define ENGINE_INTERNAL

#include "renderer.hpp"
#include "utility.hpp"

using namespace engine;


animation::animation()
{
	mTexture = nullptr;
	mDefault_frame = 0;
}

void
animation::set_loop(e_loop pLoop)
{
	mLoop = pLoop;
}

animation::e_loop
animation::get_loop()
{
	return mLoop;
}

void
animation::add_interval(frame_t pFrom, int pInterval)
{
	if (get_interval(pFrom) != pInterval)
		mSequence.push_back({ pInterval, pFrom });
}

int
animation::get_interval(frame_t pAt)
{
	int retval = 0;
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

void
animation::set_frame_count(frame_t count)
{
	mFrame_count = count;
}

frame_t
animation::get_frame_count()
{
	return mFrame_count;
}

void animation::set_frame_rect(engine::frect rect)
{
	mFrame_rect = rect;
}

engine::frect
animation::get_frame_at(frame_t at)
{
	engine::frect ret = mFrame_rect;
	ret.x += ret.w*calculate_frame(at);
	return ret;
}

fvector
animation::get_size()
{
	return mFrame_rect.get_size();
}

void
animation::set_default_frame(frame_t frame)
{
	mDefault_frame = frame;
}

int
animation::get_default_frame()
{
	return mDefault_frame;
}

void
animation::set_texture(texture& _texture)
{
	mTexture = &_texture;
}

engine::texture*
animation::get_texture()
{
	return mTexture;
}

frame_t
animation::calculate_frame(frame_t pCount)
{
	if (mFrame_count == 0)
		return 0;

	if (mLoop == animation::e_loop::linear)
		return pCount%mFrame_count;

	if (mLoop == animation::e_loop::pingpong)
		return util::pingpong_index(pCount, mFrame_count - 1);

	if (mLoop == animation::e_loop::none)
		return pCount >= mFrame_count ? mFrame_count - 1 : pCount;

	return 0;
}

animation_node::animation_node()
{
	mAnchor = anchor::topleft;
	mPlaying = false;
	mAnimation = nullptr;
	add_child(mSprite);
}

void
animation_node::set_frame(frame_t pFrame)
{
	mFrame = pFrame;
}

void
animation_node::set_animation(animation& pAnimation, bool pSwap)
{
	mAnimation = &pAnimation;
	mInterval = pAnimation.get_interval();

	if (!pSwap)
		set_frame(pAnimation.get_default_frame());

	if (pAnimation.get_texture())
		set_texture(*pAnimation.get_texture());
}

void
animation_node::set_texture(texture& pTexture)
{
	mSprite.set_texture(pTexture);
}

int
animation_node::tick()
{
	if (!mAnimation) return 0;

	int time = mClock.get_elapse().ms_i();
	if (time >= mInterval && mInterval > 0)
	{
		mFrame += time / mInterval;

		mInterval = mAnimation->get_interval(mFrame);

		mClock.restart();
	}
	return 0;
}

bool
animation_node::is_playing()
{
	return mPlaying;
}

void
animation_node::start()
{
	mClock.restart();
	mPlaying = true;
}

void
animation_node::pause()
{
	mClock.pause();
	mPlaying = false;
}

void
animation_node::stop()
{
	mPlaying = false;
	restart();
}

void
animation_node::restart()
{
	if (mAnimation)
		set_frame(mAnimation->get_default_frame());
}

void
animation_node::set_anchor(engine::anchor pAnchor)
{
	mAnchor = pAnchor;
	mSprite.set_anchor(pAnchor);
}

int
animation_node::draw(renderer &r)
{
	if (!mAnimation) return 1;
	if (!mAnimation->get_frame_count()) return 1;
	if (mPlaying) tick();

	frect rect = mAnimation->get_frame_at(mFrame);
	mSprite.set_texture_rect(rect);
	mSprite.set_anchor(mAnchor);

	mSprite.draw(r);
	return 0;
}
