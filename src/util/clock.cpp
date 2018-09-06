#include <wge/util/clock.hpp>

using namespace wge::util;

static std::chrono::time_point<std::chrono::high_resolution_clock> gProgram_start = std::chrono::high_resolution_clock::now();

float wge::util::getTime()
{
	return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - gProgram_start).count();
}

clock::clock()
{
	mStart_point = std::chrono::high_resolution_clock::now();
	mIs_paused = false;
}

float clock::get_elapse() const
{
	timepoint_t now = std::chrono::high_resolution_clock::now();
	return calc_elapse(now);
}

float clock::restart()
{
	timepoint_t now = std::chrono::high_resolution_clock::now();
	float last_elapse = calc_elapse(now);
	mStart_point = mPause_point = now;
	return last_elapse;
}

float clock::resume()
{
	timepoint_t now = std::chrono::high_resolution_clock::now();
	float last_elapse = calc_elapse(now);
	if (mIs_paused)
	{
		mIs_paused = false;
		mStart_point += now - mPause_point;
	}
	return last_elapse;
}

float clock::pause()
{
	timepoint_t now = std::chrono::high_resolution_clock::now();
	mPause_point = now;
	mIs_paused = true;
	return calc_elapse(now);
}

float clock::stop()
{
	timepoint_t now = std::chrono::high_resolution_clock::now();
	float last_elapse = calc_elapse(now);
	mStart_point = mPause_point = now;
	mIs_paused = true;
	return last_elapse;
}

bool clock::is_paused() const
{
	return mIs_paused;
}

float clock::calc_elapse(const timepoint_t & pNow) const
{
	if (mIs_paused)
		return std::chrono::duration<float>(mPause_point - mStart_point).count();
	else
		return std::chrono::duration<float>(pNow - mStart_point).count();
}
