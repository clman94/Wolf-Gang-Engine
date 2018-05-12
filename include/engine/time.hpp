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
	time_converter(const time_converter& pTime_converter);
	time_converter(float pSeconds);
	float seconds() const;
	float milliseconds() const;
	float nanoseconds() const;

	operator float() const;

private:
	float mSeconds;
};

class elapse_clock
{
public:
	elapse_clock();
	time_converter get_elapse() const;
	void resume();
	void resume(const elapse_clock& pSync);
	void pause();
	void pause(const elapse_clock& pSync); 
	bool is_paused() const;
	time_converter restart();
	void sync(const elapse_clock& pSync);

private:
	bool mPaused;
	priv::highresclock mStart_point;
	priv::highresclock mPause_point;
	priv::highresclock mLast_resume_time;
};

class timer
	: public elapse_clock
{
public:
	timer();
	void start();
	void start(float pSeconds);
	void set_duration(float pSeconds);
	bool is_reached() const;

private:
	float mSeconds;
};

class counter_clock
	: public elapse_clock
{
public:
	counter_clock();
	void set_interval(float pSeconds);
	size_t get_count() const;

private:
	float mInterval_seconds;
};

class frame_clock
{
public:
	frame_clock(float pInterval = 1);
	void pause();
	void resume();
	void set_interval(float pSeconds);
	
	float get_delta() const;
	float get_fps() const;
	time_converter get_elapse() const;

	void update();

private:
	elapse_clock mElapse_clock;
	elapse_clock mFps_clock;
	elapse_clock mDelta_clock;
	size_t mFrames;
	float mFps;
	float mInterval;
	float mDelta;
};

}

#endif