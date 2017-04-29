#ifndef ENGINE_AUDIO_HPP
#define ENGINE_AUDIO_HPP

#include <engine/resource.hpp>

#include <string>
#include <SFML/Audio.hpp>
#include <list>
#include <memory>
#include <engine/resource_pack.hpp>

// Wrapper class for sfml sound

namespace engine
{
class sound_buffer :
	public resource
{
public:
	void set_sound_source(const std::string& pFilepath);
	bool load();
	bool unload();
private:
	std::string mSound_source;
	std::unique_ptr<sf::SoundBuffer> mSFML_buffer;

	friend class sound;
};

class sound
{
	std::shared_ptr<sound_buffer> mSound_buffer;
	sf::Sound s;
public:
	sound() {}
	sound(std::shared_ptr<sound_buffer> buf)
	{
		set_buffer(buf);
	}
	void set_buffer(std::shared_ptr<sound_buffer> buf)
	{
		mSound_buffer = buf;
		s.setBuffer(*buf->mSFML_buffer);
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
	void spawn(std::shared_ptr<sound_buffer> pBuffer, float pVolume = 100, float pPitch = 1);
	void stop_all();
};

class sound_stream
{
public:
	sound_stream();

	bool open(const std::string& path);
	bool open(const std::string& path, pack_stream_factory& mPack);
	void play();
	void stop();
	void pause();
	void set_pitch(float pitch);
	void set_loop(bool loop);
	void set_volume(float volume);
	float get_volume();
	bool is_playing();
	float get_position();
	void set_position(float pSeconds);

	float get_duration();

	bool is_valid();

private:

	struct sfml_stream_ : public sf::InputStream
	{
		engine::pack_stream stream;
		virtual sf::Int64 read(void* pData, sf::Int64 pSize);
		virtual sf::Int64 seek(sf::Int64 pPosition);
		virtual sf::Int64 tell();
		virtual sf::Int64 getSize();
	} sfml_stream;

	sf::Music mSFML_music;
	bool valid;

};


}

#endif // !ENGINE_AUDIO_HPP
