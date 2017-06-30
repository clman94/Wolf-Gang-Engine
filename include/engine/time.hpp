#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

namespace engine
{

namespace priv
{
typedef std::chrono::time_point<std::chrono::high_resolution_clock> highresclock;
}

class time_converter
{
public:
	time_converter(float pSeconds);
	float seconds() const;
	float milliseconds() const;
	float nanoseconds() const;

	operator float() const;

private:
	float mSeconds;
};

class clock
{
	bool play;
	priv::highresclock start_point;
	priv::highresclock pause_point;
public:
	clock();
	time_converter get_elapse() const;
	void start();
	void pause();
	time_converter restart();
};

class timer
{
public:
	void start();
	void start(float pSeconds);
	void set_duration(float pSeconds);
	bool is_reached() const;

private:
	priv::highresclock mStart_point;
	float mSeconds;
};

class counter_clock
{
public:
	void start();
	void set_interval(float pInterval);
	size_t get_count() const;

private:
	priv::highresclock mStart_point;
	float mInterval;
};

class frame_clock
{
public:
	frame_clock(float pInterval = 1);

	void set_interval(float pSeconds);
	
	float get_delta() const;
	float get_fps() const;

	void tick();

private:
	clock mFps_clock;
	clock mDelta_clock;
	size_t mFrames;
	float mFps;
	float mInterval;
	float mDelta;
};

}

#endif