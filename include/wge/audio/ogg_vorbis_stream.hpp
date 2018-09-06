#pragma once

#include <string>

struct stb_vorbis;

namespace wge::audio
{

class ogg_vorbis_stream
{
public:
	ogg_vorbis_stream();
	~ogg_vorbis_stream();

	// Open a file as a stream
	void open(const std::string& pPath);
	// Close the stream
	void close();

	// Returns the amount of channels in this file
	int get_channel_count() const;
	// Returns the sample rate of this file
	int get_sample_rate() const;
	// Returns the duration of the sound in seconds
	float get_duration() const;
	// Get total amount of sample in the stream.
	int get_samples() const;

	// The channel data in pSamples is interlaced if there is more than one channel.
	//   [channel1_sample1, channel2_sample1, channel1_sample2, channel2_sample2, etc...]
	// Returns the total amount of samples actually read.
	int read(short* pSamples, int mSize);

	// Seek to the beginning of the stream
	void seek_beginning();
	// Seek to a specific time in the stream.
	// Returns the sample position after the seek.
	int seek_time(float pSeconds);
	// Seek to a specific sample in the stream.
	// Returns the sample position after the seek.
	int seek_sample(int pSample);

	// Returns the time, in seconds, of the current stream position
	float tell_time() const;
	// Returns the current sample position
	int tell_sample() const;

	// Returns true if this stream is loaded and ready.
	bool is_valid() const;

private:
	stb_vorbis* mVorbis_stream;
	float mDuration_seconds;
	int mDuration_samples;
	int mChannels;
	int mSample_rate;
};

}