#include <engine/time.hpp>
#include <cmath>
#include <cassert>

using namespace engine;

// Seconds
float time_converter::seconds() const
{
	return mSeconds;
}

// Microseconds
float time_converter::milliseconds() const
{
	return mSeconds * 1000;
}

float time_converter::nanoseconds() const
{
	return mSeconds * 1000 * 1000;
}

time_converter::operator float() const
{
	return mSeconds;
}

time_converter::time_converter(const time_converter & pTime_converter)
{
	mSeconds = pTime_converter.mSeconds;
}

time_converter::time_converter(float pSeconds)
{
	mSeconds = pSeconds;
}

elapse_clock::elapse_clock()
{
	mPaused = false;
	mStart_point = std::chrono::high_resolution_clock::now();
	mLast_resume_time = mStart_point;
}

time_converter elapse_clock::get_elapse() const
{
	if (mPaused)
		return std::chrono::duration<float>(mPause_point - mStart_point).count();
	std::chrono::time_point<std::chrono::high_resolution_clock> end_point = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsed_seconds = end_point - mStart_point;
	return elapsed_seconds.count();
}

void elapse_clock::resume()
{
	mLast_resume_time = std::chrono::high_resolution_clock::now();
	if (mPaused)
		mStart_point += mLast_resume_time - mPause_point;
	mPaused = false;
}

void elapse_clock::resume(const elapse_clock & pSync)
{
	mLast_resume_time = pSync.mLast_resume_time;
	if (mPaused)
		mStart_point += mLast_resume_time - mPause_point;
	mPaused = false;
}

void elapse_clock::pause()
{
	mPaused = true;
	mPause_point = std::chrono::high_resolution_clock::now();
}

void elapse_clock::pause(const elapse_clock & pSync)
{
	assert(pSync.mPaused);
	mPause_point = pSync.mPause_point;
	mPaused = true;
}

bool elapse_clock::is_paused() const
{
	return mPaused;
}

time_converter elapse_clock::restart()
{
	const time_converter elapse_time(get_elapse());
	mStart_point = std::chrono::high_resolution_clock::now();
	if (mPaused)
		mPause_point = mStart_point;
	else
		mLast_resume_time = mStart_point;
	return elapse_time;
}

void elapse_clock::sync(const elapse_clock & pSync)
{
	mStart_point = pSync.mStart_point;
}

timer::timer()
{
	pause();
	restart();
	mSeconds = 0;
}

void timer::start()
{
	restart();
	resume();
}

void timer::start(float pSeconds)
{
	set_duration(pSeconds);
	restart();
	resume();
}

void timer::set_duration(float pSeconds)
{
	if (pSeconds <= 0)
		return;
	mSeconds = pSeconds;
}

bool timer::is_reached() const
{
	return get_elapse().seconds() >= mSeconds;
}

counter_clock::counter_clock()
{
	mInterval_seconds = 0;
}

void counter_clock::set_interval(float pSeconds)
{
	if (pSeconds < 0)
		throw "Bad time";
	mInterval_seconds = pSeconds;
}

size_t counter_clock::get_count() const
{
	if (mInterval_seconds <= 0)
		return 0;
	return static_cast<size_t>(std::floor(get_elapse().seconds() / mInterval_seconds));
}

frame_clock::frame_clock(float pInterval)
{
	mFps = 0;
	mFrames = 0;
	mInterval = pInterval;
	mDelta = 0;

}

void frame_clock::pause()
{
	mElapse_clock.pause();
	mFps_clock.pause(mElapse_clock);
	mDelta_clock.pause(mElapse_clock);
}

void frame_clock::resume()
{
	mElapse_clock.resume();
	mFps_clock.resume(mElapse_clock);
	mDelta_clock.resume(mElapse_clock);
}

void frame_clock::set_interval(float pSeconds)
{
	mInterval = pSeconds;
}

float frame_clock::get_delta() const
{
	return mDelta;
}

float frame_clock::get_fps() const
{
	return mFps;
}

time_converter frame_clock::get_elapse() const
{
	return mElapse_clock.get_elapse();
}

void frame_clock::update()
{
	++mFrames;
	float time = mFps_clock.get_elapse().seconds();
	if (time >= mInterval)
	{
		mFps = mFrames / time;
		mFrames = 0;
		mFps_clock.restart();
	}

	mDelta = mDelta_clock.get_elapse().seconds();
	mDelta_clock.restart();
}
