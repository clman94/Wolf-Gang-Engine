#pragma once

#include <chrono>

namespace wge::util
{

// Get time since start of application in seconds.
// This uses the highest resolution possible (normally in nanoseconds).
float getTime();

class clock
{
private:
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint_t;

public:
	clock();

	// Returns the current elapsed time in seconds
	float get_elapse() const;

	// Restarts the clock at 0 seconds
	float restart();
	// Resumes the clock if it was paused
	float resume();
	// Pauses the clock
	float pause();
	// Pause and rewind the clock
	float stop();

	bool is_paused() const;

private:
	float calc_elapse(const timepoint_t& pNow) const;

	timepoint_t mStart_point;
	timepoint_t mPause_point;
	bool mIs_paused;
};

} // namespace wge::util
