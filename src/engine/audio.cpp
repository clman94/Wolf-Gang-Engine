#include <engine/audio.hpp>
#include <engine/log.hpp>
#include <engine/filesystem.hpp>
using namespace engine;

engine::sound * engine::sound_spawner::get_new_sound_object()
{
	for (auto &i : mSounds)
		if (!i.is_playing())
			return &i;

	mSounds.emplace_back();
	return &mSounds.back();
}

void sound_spawner::spawn(std::shared_ptr<sound_file> pBuffer, float pVolume, float pPitch)
{
	engine::sound* sound_object = get_new_sound_object();

	sound_object->set_sound_resource(pBuffer);
	sound_object->set_volume(pVolume);
	sound_object->set_pitch(pPitch);
	sound_object->play();
}
void sound_spawner::stop_all()
{
	for (auto &i : mSounds)
		i.stop();
	mSounds.clear();
}

inline engine::sound::sound()
{
	mReady = false;
}

void sound::set_sound_resource(std::shared_ptr<sound_file> pResource)
{
	stop();
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

	mSource = pResource;

	mReady = true;
}


// Sorry for the nasty duplication here
// SFML, unfortunately, doesn't provide any alternatives

void sound::play()
{
	if (!mSource)
		return;
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.play();
	else
		mSFML_streamless_sound.play();
}

void sound::stop()
{
	if (!mSource)
		return;
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.stop();
	else
		mSFML_streamless_sound.stop();
}

void sound::pause()
{
	if (!mSource)
		return;
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.pause();
	else
		mSFML_streamless_sound.pause();
}

void sound::set_pitch(float pPitch)
{
	mSFML_stream_sound.setPitch(pPitch);
	mSFML_streamless_sound.setPitch(pPitch);
}

float sound::get_pitch() const
{
	if (!mSource)
		return 0;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getPitch();
	else
		return mSFML_streamless_sound.getPitch();
}

void sound::set_loop(bool pLoop)
{
	mSFML_stream_sound.setLoop(pLoop);
	mSFML_streamless_sound.setLoop(pLoop);
}

float sound::get_loop() const
{
	if (!mSource)
		return false;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getLoop();
	else
		return mSFML_streamless_sound.getLoop();
}

void sound::set_volume(float pVolume)
{
	float volume = util::clamp(pVolume, 0.f, 1.f)*100;
	mSFML_stream_sound.setVolume(volume);
	mSFML_streamless_sound.setVolume(volume);
}

float sound::get_volume() const
{
	if (!mSource)
		return 0;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getVolume() / 100;
	else
		return mSFML_streamless_sound.getVolume() / 100;
}

bool sound::is_playing() const
{
	if (!mSource)
		return false;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getStatus() == sf::SoundSource::Playing;
	else
		return mSFML_streamless_sound.getStatus() == sf::SoundSource::Playing;
}
float sound::get_duration() const
{
	if (!mSource)
		return 0;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getDuration().asSeconds();
	else
		return mSFML_streamless_sound.getBuffer()->getDuration().asSeconds();
}
float sound::get_playoffset() const
{
	if (!mSource)
		return 0;
	if (mSource->mRequires_streaming)
		return mSFML_stream_sound.getPlayingOffset().asSeconds();
	else
		return mSFML_streamless_sound.getPlayingOffset().asSeconds();
}
void sound::set_playoffset(float pSeconds)
{
	if (!mSource)
		return;
	if (mSource->mRequires_streaming)
		mSFML_stream_sound.setPlayingOffset(sf::seconds(pSeconds));
	else
		mSFML_streamless_sound.setPlayingOffset(sf::seconds(pSeconds));
}
inline sf::Int64 sound::sfml_stream_::read(void * pData, sf::Int64 pSize)
{
	if (!stream.is_valid())
		return -1;
	return stream.read((char*)pData, pSize);
}

inline sf::Int64 sound::sfml_stream_::seek(sf::Int64 pPosition)
{
	if (!stream.is_valid())
		return -1;
	return stream.seek(pPosition) ? pPosition : -1;
}

inline sf::Int64 sound::sfml_stream_::tell()
{
	if (!stream.is_valid())
		return -1;
	return stream.tell();
}

inline sf::Int64 sound::sfml_stream_::getSize()
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
	set_loaded(true);
	return true;
}

bool sound_file::unload()
{
	mSFML_buffer = sf::SoundBuffer();
	set_loaded(false);
	return true;
}

void sound_file::set_filepath(const std::string & pPath)
{
	mSound_source = pPath;
}
