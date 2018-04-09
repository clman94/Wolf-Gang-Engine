#ifndef ENGINE_AUDIO_HPP
#define ENGINE_AUDIO_HPP

#include <engine/node.hpp>
#include <engine/resource.hpp>
#include <engine/resource_pack.hpp>

#include <string>
#include <list>
#include <memory>

#include <SFML/Audio.hpp>

namespace engine
{

const std::string sound_file_restype = "audio";

class sound_file :
	public resource
{
public:
	static const size_t streaming_threshold = 1000000;

	bool load();
	bool unload();

	const std::string& get_type() const override
	{
		return sound_file_restype;
	}

	void set_filepath(const std::string& pPath);

private:
	bool load_buffer();
	std::string mSound_source;
	sf::SoundBuffer mSFML_buffer;

	bool mBuffer_loaded;

	friend class sound;
};

class mixer;

class sound
{
public:
	sound();
	~sound();

	void set_sound_resource(std::shared_ptr<sound_file> pResource);

	void play();
	void stop();
	void pause();

	void set_pitch(float pPitch);
	float get_pitch() const;

	void set_loop(bool pLoop);
	bool get_loop() const;

	void set_volume(float pVolume);
	float get_volume() const;

	bool is_playing() const;
	float get_duration() const;

	float get_playoffset() const;
	void set_playoffset(float pSeconds);

	bool attach_mixer(mixer& pMixer);
	bool detach_mixer();

	void set_mono(bool pIs_mono);
	bool get_mono() const;

private:
	mixer* mMixer;
	void update_volume();

	float mVolume;

	std::shared_ptr<sound_file> mSource;

	bool mMono;

	bool mReady;

	struct sfml_stream : public sf::InputStream
	{
		engine::pack_stream stream;
		virtual sf::Int64 read(void* pData, sf::Int64 pSize);
		virtual sf::Int64 seek(sf::Int64 pPosition);
		virtual sf::Int64 tell();
		virtual sf::Int64 getSize();
	} mSfml_stream;

	sf::Music mSFML_stereo_source;
	sf::Sound mSFML_mono_source;

	friend class mixer;
};

class mixer
{
public:
	mixer();
	~mixer();

	void set_master_volume(float pVolume);
	float get_master_volume() const;

	bool add(sound& pSound);
	bool remove(sound& pSound);
private:
	float mMaster_volume;
	std::vector<sound*> mSounds;
};

class sound_spawner
{
public:
	sound_spawner();

	void spawn(std::shared_ptr<sound_file> pBuffer, float pVolume = 1, float pPitch = 1);
	void stop_all();

	void attach_mixer(mixer& pMixer);
	void detach_mixer();

private:
	mixer * mMixer;
	engine::sound * new_sound_object();
	std::list<sound> mSounds;
};


}

#endif // !ENGINE_AUDIO_HPP
