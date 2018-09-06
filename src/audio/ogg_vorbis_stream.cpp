#include <wge/audio/ogg_vorbis_stream.hpp>

#include <iostream>

#include <stb/stb_vorbis.h>

using namespace wge::audio;

ogg_vorbis_stream::ogg_vorbis_stream()
{
	mVorbis_stream = nullptr;
	mDuration_seconds = 0;
	mDuration_samples = 0;
	mChannels = 0;
	mSample_rate = 0;
}

ogg_vorbis_stream::~ogg_vorbis_stream()
{
	close();
}

void ogg_vorbis_stream::open(const std::string & pPath)
{
	int error = 0;
	mVorbis_stream = stb_vorbis_open_filename(pPath.c_str(), &error, NULL);
	if (!mVorbis_stream)
	{
		std::cout << "Error opening vorbis file\n";
		return;
	}

	stb_vorbis_info vorbis_info = stb_vorbis_get_info(mVorbis_stream);
	mChannels = vorbis_info.channels;
	mSample_rate = vorbis_info.sample_rate;
	mDuration_seconds = stb_vorbis_stream_length_in_seconds(mVorbis_stream);
	mDuration_samples = stb_vorbis_stream_length_in_samples(mVorbis_stream);
}

void ogg_vorbis_stream::close()
{
	stb_vorbis_close(mVorbis_stream);
	mVorbis_stream = NULL;
	mDuration_seconds = 0;
	mDuration_samples = 0;
	mChannels = 0;
	mSample_rate = 0;
}

int ogg_vorbis_stream::get_channel_count() const
{
	return mChannels;
}

int ogg_vorbis_stream::get_sample_rate() const
{
	return mSample_rate;
}

float ogg_vorbis_stream::get_duration() const
{
	return mDuration_seconds;
}

int ogg_vorbis_stream::get_samples() const
{
	return mDuration_samples;
}

int ogg_vorbis_stream::read(short * pSamples, int mSize)
{
	int amount = stb_vorbis_get_samples_short_interleaved(mVorbis_stream, mChannels, pSamples, mSize);
	return amount * mChannels;
}

void ogg_vorbis_stream::seek_beginning()
{
	seek_sample(0);
}

int ogg_vorbis_stream::seek_time(float pSeconds)
{
	return stb_vorbis_seek(mVorbis_stream, mSample_rate*pSeconds);
}

int ogg_vorbis_stream::seek_sample(int pSample)
{
	return stb_vorbis_seek(mVorbis_stream, pSample);
}

float ogg_vorbis_stream::tell_time() const
{
	return static_cast<float>(stb_vorbis_get_sample_offset(mVorbis_stream)) / static_cast<float>(mSample_rate);
}

int ogg_vorbis_stream::tell_sample() const
{
	return stb_vorbis_get_sample_offset(mVorbis_stream);
}

bool ogg_vorbis_stream::is_valid() const
{
	return mVorbis_stream != NULL;
}
