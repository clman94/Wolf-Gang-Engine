#include <engine/time.hpp>

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

time_converter::time_converter(float pSeconds)
{
	mSeconds = pSeconds;
}

clock::clock()
{
	play = true;
	start_point = std::chrono::high_resolution_clock::now();
}

time_converter clock::get_elapse() const
{
	std::chrono::time_point<std::chrono::high_resolution_clock> end_point = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsed_seconds = end_point - start_point;
	return elapsed_seconds.count();
}

void clock::start()
{
	if (!play)
		start_point += std::chrono::high_resolution_clock::now() - pause_point;
	play = true;
}

void clock::pause()
{
	play = false;
	pause_point = std::chrono::high_resolution_clock::now();
}

time_converter clock::restart()
{
	const time_converter elapse_time(get_elapse());
	start_point = std::chrono::high_resolution_clock::now();
	return elapse_time;
}

void timer::start()
{
	mStart_point = std::chrono::high_resolution_clock::now();
}

void timer::start(float pSeconds)
{
	set_duration(pSeconds);
	start();
}

void timer::set_duration(float pSeconds)
{
	if (pSeconds <= 0)
		return;
	mSeconds = pSeconds;
}

bool timer::is_reached() const
{
	std::chrono::duration<float> time = std::chrono::high_resolution_clock::now() - mStart_point;
	return time.count() >= mSeconds;
}

void counter_clock::start()
{
	mStart_point = std::chrono::high_resolution_clock::now();
}

void counter_clock::set_interval(float pInterval)
{
	if (pInterval <= 0)
		throw "Bad time";
	mInterval = pInterval;
}

size_t counter_clock::get_count() const
{
	std::chrono::duration<float> time = std::chrono::high_resolution_clock::now() - mStart_point;
	return static_cast<size_t>(std::floor(time.count() / mInterval));
}

frame_clock::frame_clock(float pInterval)
{
	mFps = 0;
	mFrames = 0;
	mInterval = pInterval;
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

void frame_clock::tick()
{
	++mFrames;
	auto time = mFps_clock.get_elapse().seconds();
	if (time >= mInterval)
	{
		mFps = mFrames / time;
		mFrames = 0;
		mFps_clock.restart();
	}

	mDelta = mDelta_clock.get_elapse().seconds();
	mDelta_clock.restart();
}
