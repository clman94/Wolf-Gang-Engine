#include <engine/audio.hpp>
#include <engine/log.hpp>
#include <engine/filesystem.hpp>
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

bool sound_stream::open(const std::string & path, const pack_stream_factory& mPack)
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

engine::sound * engine::sound_spawner::get_new_sound_object()
{
	for (auto &i : mSounds)
		if (!i.is_playing())
			return &i;

	mSounds.emplace_back();
	return &mSounds.back();
}

void sound_spawner::spawn(std::shared_ptr<sound_buffer> pBuffer, float pVolume, float pPitch)
{
	engine::sound* sound_object = get_new_sound_object();

	sound_object->set_buffer(pBuffer);
	sound_object->set_volume(pVolume);
	sound_object->set_pitch(pPitch);
	sound_object->play();
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

sound::sound(std::shared_ptr<sound_buffer> buf)
{
	set_buffer(buf);
}

void sound::set_buffer(std::shared_ptr<sound_buffer> buf)
{
	mSound_buffer = buf;
	s.setBuffer(*buf->mSFML_buffer);
}

void sound::play()
{
	s.play();
}

void sound::stop()
{
	s.stop();
}

void sound::pause()
{
	s.pause();
}

void sound::set_pitch(float pitch)
{
	s.setPitch(pitch);
}

void sound::set_loop(bool loop)
{
	s.setLoop(loop);
}

void sound::set_volume(float volume)
{
	s.setVolume(volume);
}

bool sound::is_playing()
{
	return s.getStatus() == s.Playing;
}

void new_sound::set_sound_resource(std::shared_ptr<sound_file> pResource)
{
	if (pResource->requires_streaming())
	{
		if (pResource->mPack)
		{
			mSfml_stream.stream = pResource->mPack->create_stream(pResource->mSound_source);
			if (!mSfml_stream.stream.is_valid())
			{
				logger::error("Failed to load stream '" + pResource->mSound_source + "' from pack");

				mReady = false;
				return;
			}
		}
		else
		{
			if (!mSFML_stream_sound.openFromFile(pResource->mSound_source))
			{
				logger::error("Failed to load stream from '" + pResource->mSound_source + "'");

				mReady = false;
				return;
			}
		}
	}
	else
	{
		mSFML_streamless_sound.setBuffer(pResource->mSFML_buffer);
	}

	mReady = true;
}


// Sorry for the nasty duplication here
// SFML, unfortunately, doesn't provide any alternatives

void new_sound::play()
{
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.play();
	else
		mSFML_streamless_sound.play();
}

void new_sound::stop()
{
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.stop();
	else
		mSFML_streamless_sound.stop();
}

void new_sound::pause()
{
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.pause();
	else
		mSFML_streamless_sound.pause();
}

void new_sound::set_pitch(float pPitch)
{
	mSFML_stream_sound.setPitch(pPitch);
	mSFML_streamless_sound.setPitch(pPitch);
}

float new_sound::get_pitch() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getPitch();
	else
		return mSFML_streamless_sound.getPitch();
}

void new_sound::set_loop(bool pLoop)
{
	mSFML_stream_sound.setLoop(pLoop);
	mSFML_streamless_sound.setLoop(pLoop);
}

float new_sound::get_loop() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getLoop();
	else
		return mSFML_streamless_sound.getLoop();
}

void new_sound::set_volume(float pVolume)
{
	mSFML_stream_sound.setVolume(pVolume*100);
	mSFML_streamless_sound.setLoop(pVolume*100);
}

float new_sound::get_volume() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getVolume() / 100;
	else
		return mSFML_streamless_sound.getVolume() / 100;
}

bool new_sound::is_playing() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getStatus() == sf::SoundSource::Playing;
	else
		return mSFML_streamless_sound.getStatus() == sf::SoundSource::Playing;
}
float new_sound::get_duration() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getDuration().asSeconds();
	else
		return mSFML_streamless_sound.getBuffer()->getDuration().asSeconds();
}
float new_sound::get_playoffset() const
{
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getPlayingOffset().asSeconds();
	else
		return mSFML_streamless_sound.getPlayingOffset().asSeconds();
}
void new_sound::set_playoffset(float pSeconds)
{
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.setPlayingOffset(sf::seconds(pSeconds));
	else
		mSFML_streamless_sound.setPlayingOffset(sf::seconds(pSeconds));
}
inline sf::Int64 new_sound::sfml_stream_::read(void * pData, sf::Int64 pSize)
{
	if (!stream.is_valid())
		return -1;
	return stream.read((char*)pData, pSize);
}

inline sf::Int64 new_sound::sfml_stream_::seek(sf::Int64 pPosition)
{
	if (!stream.is_valid())
		return -1;
	return stream.seek(pPosition) ? pPosition : -1;
}

inline sf::Int64 new_sound::sfml_stream_::tell()
{
	if (!stream.is_valid())
		return -1;
	return stream.tell();
}

inline sf::Int64 new_sound::sfml_stream_::getSize()
{
	if (!stream.is_valid())
		return -1;
	return stream.size();
}

bool sound_file::requires_streaming() const
{
	return mRequires_streaming;
}

bool sound_file::load()
{
	if (!(mRequires_streaming = (fs::file_size(mSound_source) >= streaming_threshold)))
	{
		if (!is_loaded())
		{
			if (mPack)
			{
				auto data = mPack->read_all(mSound_source);
				set_loaded(mSFML_buffer.loadFromMemory(&data[0], data.size()));
			}
			else
				set_loaded(mSFML_buffer.loadFromFile(mSound_source));
		}
		return is_loaded();
	}
	return true;
}

bool sound_file::unload()
{
	mSFML_buffer = sf::SoundBuffer();
	return true;
}

void sound_file::set_filepath(const std::string & pPath)
{
	mSound_source = pPath;
}
