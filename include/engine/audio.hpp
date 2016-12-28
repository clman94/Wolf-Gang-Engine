#include <string>
#include <SFML\Audio.hpp>
#include <list>
// Wrapper class for sfml sound

namespace engine
{
class sound_buffer
{
	sf::SoundBuffer buf;
public:
	bool load(const std::string path)
	{
		return buf.loadFromFile(path);
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

/// A pool for sound objects
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


}
