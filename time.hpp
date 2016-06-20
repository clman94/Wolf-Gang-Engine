#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

namespace engine
{
typedef double time_t;

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
	std::chrono::time_point<std::chrono::system_clock> start, end;
public:
	clock()
	{
		start = std::chrono::system_clock::now();
	}
	utime get_elapse()
	{
		end = std::chrono::system_clock::now();
		std::chrono::duration<time_t> elapsed_seconds = end - start;
		return elapsed_seconds.count();
	}
	utime restart()
	{
		utime elapse_time = get_elapse();
		start = std::chrono::system_clock::now();
		return elapse_time;
	}
};

}

#endif