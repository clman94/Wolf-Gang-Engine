#include <string>
#include <SFML\Audio.hpp>
#include <list>
#include <cmath>

// Wrapper class for sfml sound

namespace engine
{
class sound_buffer
{
	sf::SoundBuffer buf;
public:
	int load(const std::string path)
	{
		return !buf.loadFromFile(path);
	}
	friend class sound;
};

class sound
{
	sf::Sound s;
public:
	sound(){}
	sound(const sound_buffer& buf)
	{
		set_buffer(buf);
	}
	void set_buffer(const sound_buffer& buf)
	{
		s.setBuffer(buf.buf);
	}
	void play()
	{
		s.play();
	}
	void stop()
	{
		s.stop();
	}
	void pause()
	{
		s.pause();
	}
	void set_pitch(float pitch)
	{
		s.setPitch(pitch);
	}
	void set_loop(bool loop)
	{
		s.setLoop(loop);
	}
	void set_volume(float volume)
	{
		s.setVolume(volume);
	}
	bool is_playing()
	{
		return s.getStatus() == s.Playing;
	}
};

class sound_spawner
{
	std::list<sound> mSounds;
public:
	void spawn(sound_buffer& pBuffer, float pVolume = 100, float pPitch = 1)
	{
		for (auto &i : mSounds)
		{
			if (!i.is_playing())
			{
				i.set_buffer(pBuffer);
				i.play();
				return;
			}
		}
		mSounds.emplace_back(pBuffer);
		auto& newsound = mSounds.back();
		newsound.set_volume(pVolume);
		newsound.set_pitch(pPitch);
		newsound.play();
	}
	void stop_all()
	{
		for (auto &i : mSounds)
		{
			i.stop();
		}
		mSounds.clear();
	}
};

class sound_stream
{
	sf::Music s;
	bool valid;
public:
	sound_stream()
	{
		valid = false;
	}
	int open(const std::string& path)
	{
		valid = s.openFromFile(path);
		return !valid;
	}
	void play()
	{
		s.play();
	}
	void stop()
	{
		s.stop();
	}
	void pause()
	{
		s.pause();
	}
	void set_pitch(float pitch)
	{
		s.setPitch(pitch);
	}
	void set_loop(bool loop)
	{
		s.setLoop(loop);
	}
	void set_volume(float volume)
	{
		s.setVolume(volume);
	}
	bool is_playing()
	{
		return s.getStatus() == s.Playing;
	}
	float get_position()
	{
		return s.getPlayingOffset().asSeconds();
	}
	float get_duration()
	{
		return s.getDuration().asSeconds();
	}

	bool is_valid()
	{
		return valid;
	}
};


// ################
// Experimentation
// ################

// Calculate frequency of note from C5
static float note_freq(int halfsteps)
{
	const float a = std::pow(2.f, 1.f / 12);
	return 440 * std::pow(a, halfsteps + 3);
}

class freq_sequence
{
	struct entry
	{
		size_t start, duration;
		float freq, volume;
		int voice;
	};
	std::vector<entry> seq;
public:
	freq_sequence();
	freq_sequence(const freq_sequence& a);
	void add(const freq_sequence& fs, float start, int voice = 0);
	void add(int note, size_t sample, float duration, float volume, int voice = 0);
	void add(int note, float start, float duration, float volume, int voice = 0);
	void append(int note, float duration, float volume, int voice = 0);
	void append(const freq_sequence& fs, int voice = 0);
	const freq_sequence snip(size_t s_start, size_t s_duration);
	friend class sample_buffer;
};

class sample_buffer
{
	std::vector<sf::Int16> samples;

public:
	sample_buffer();
	sample_buffer(sample_buffer& m);
	sample_buffer(sample_buffer&& m);
	sample_buffer& operator=(const sample_buffer& r);
	static sample_buffer mix(sample_buffer& buf1, sample_buffer& buf2);
	int mix_with(sample_buffer& buf, int pos);

	enum wave_type
	{
		wave_sine,
		wave_saw,
		wave_triangle,
		wave_noise
	};

	static void generate(sample_buffer& buf, int wave, float f, float v, size_t start = 0, size_t duration = 1);
	static void generate(sample_buffer& buf, int wave, const freq_sequence& seq, int voice = 0);
	friend class sample_mix;
};

class sample_mix
{
	struct channel
	{
		freq_sequence* seq;
		int voice, wave;
	};
	std::vector<channel> channels;
	sample_buffer mix;
	sf::SoundBuffer buffer;
	sf::Sound output, output2;
	int c_section;

public:
	sample_mix();
	void generate_section(size_t start, size_t duration);
	const std::vector<signed short>& get_buffer();
	void add_mix(freq_sequence& seq, int wave, int voice);
	void setup();
	void play();
	static void test_song();
};

}
