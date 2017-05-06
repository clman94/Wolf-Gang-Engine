#include <engine/audio.hpp>
using namespace engine;

void sound_buffer::set_sound_source(const std::string & pFilepath)
{
	mSound_source = pFilepath;
}

bool sound_buffer::load()
{
	if (!is_loaded())
	{
		mSFML_buffer.reset(new sf::SoundBuffer());
		if (mPack)
		{
			auto data = mPack->read_all(mSound_source);
			set_loaded(mSFML_buffer->loadFromMemory(&data[0], data.size()));
		}
		else
			set_loaded(mSFML_buffer->loadFromFile(mSound_source));
	}
	return is_loaded();
}

bool sound_buffer::unload()
{
	mSFML_buffer.reset();
	set_loaded(false);
	return true;
}

sound_stream::sound_stream()
{
	valid = false;
}

bool sound_stream::open(const std::string & path)
{
	if (sfml_stream.stream.is_valid())
		sfml_stream.stream.close();
	return mSFML_music.openFromFile(path);
}

bool sound_stream::open(const std::string & path, pack_stream_factory & mPack)
{
	sfml_stream.stream = mPack.create_stream(path);
	sfml_stream.stream.open();
	if (!sfml_stream.stream.is_valid())
		return false;
	mSFML_music.openFromStream(sfml_stream);
	return true;
}

void sound_stream::play()
{
	mSFML_music.play();
}

void sound_stream::stop()
{
	mSFML_music.stop();
}

void sound_stream::pause()
{
	mSFML_music.pause();
}

void sound_stream::set_pitch(float pitch)
{
	mSFML_music.setPitch(pitch);
}

void sound_stream::set_loop(bool loop)
{
	mSFML_music.setLoop(loop);
}

void sound_stream::set_volume(float volume)
{
	mSFML_music.setVolume(util::clamp(volume, 0.f, 100.f));
}

float sound_stream::get_volume()
{
	return mSFML_music.getVolume();
}

bool sound_stream::is_playing()
{
	return mSFML_music.getStatus() == mSFML_music.Playing;
}

float sound_stream::get_position()
{
	return mSFML_music.getPlayingOffset().asSeconds();
}

void sound_stream::set_position(float pSeconds)
{
	return mSFML_music.setPlayingOffset(sf::seconds(pSeconds));
}

float sound_stream::get_duration()
{
	return mSFML_music.getDuration().asSeconds();
}

bool sound_stream::is_valid()
{
	return valid;
}

void sound_spawner::spawn(std::shared_ptr<sound_buffer> pBuffer, float pVolume, float pPitch)
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

void sound_spawner::stop_all()
{
	for (auto &i : mSounds)
	{
		i.stop();
	}
	mSounds.clear();
}

inline sf::Int64 sound_stream::sfml_stream_::read(void * pData, sf::Int64 pSize)
{
	if (!stream.is_valid())
		return -1;
	return stream.read((char*)pData, pSize);
}

inline sf::Int64 sound_stream::sfml_stream_::seek(sf::Int64 pPosition)
{
	if (!stream.is_valid())
		return -1;
	return stream.seek(pPosition) ? pPosition : -1;
}

inline sf::Int64 sound_stream::sfml_stream_::tell()
{
	if (!stream.is_valid())
		return -1;
	return stream.tell();
}

inline sf::Int64 sound_stream::sfml_stream_::getSize()
{
	if (!stream.is_valid())
		return -1;
	return stream.size();
}
