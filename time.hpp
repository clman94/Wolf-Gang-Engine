#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

namespace engine
{
typedef float time_t;

// Universal time converter for clock
class utime
{
	time_t t;
public:


	// Seconds
	time_t s()
	{
		return t;
	}

	// Seconds as integer
	int s_i()
	{
		return (int)t;
	}

	// Microseconds
	time_t ms()
	{
		return t * 1000;
	}

	// Microseconds as integer
	int ms_i()
	{
		return (int)(t * 1000);
	}

	// Nanoseconds
	time_t ns()
	{
		return t*1000*1000;
	}

	utime(time_t A)
	{
		t = A;
	}
};

class clock
{
	bool play;
	std::chrono::time_point<std::chrono::system_clock> 
		start_point,
		pause_point;
public:
	clock()
	{
		play = true;
		start_point = std::chrono::system_clock::now();
	}
	utime get_elapse()
	{
		std::chrono::time_point<std::chrono::system_clock> end_point = std::chrono::system_clock::now();
		std::chrono::duration<time_t> elapsed_seconds = end_point - start_point;
		return elapsed_seconds.count();
	}
	void start()
	{
		if (!play)
			start_point += std::chrono::system_clock::now() - pause_point;
		play = true;
	}
	void pause()
	{
		play = false;
		pause_point = std::chrono::system_clock::now();
	}
	utime restart()
	{
		utime elapse_time = get_elapse();
		start_point = std::chrono::system_clock::now();
		return elapse_time;
	}
};

class timer
{
	std::chrono::time_point<std::chrono::system_clock>
		start_point;
	time_t seconds;
public:
	void start_timer(float sec)
	{
		seconds = sec;
		start_point = std::chrono::system_clock::now();
	}

	bool is_reached()
	{
		std::chrono::duration<time_t> time = std::chrono::system_clock::now() - start_point;
		return time.count() >= seconds;
	}
};

class fps_clock
{
	engine::clock clock;
	unsigned int frames;
	float fps;
	float interval;
public:
	fps_clock(float _interval = 1)
	{
		fps = 0;
		frames = 0;
		interval = _interval;
	}

	void set_interval(float seconds)
	{
		interval = seconds;
	}

	float get_fps()
	{
		return fps;
	}

	void update()
	{
		++frames;
		auto time = clock.get_elapse().s();
		if (time >= interval)
		{
			fps = frames / time;
			frames = 0;
			clock.restart();
		}
	}
};

}

#endif