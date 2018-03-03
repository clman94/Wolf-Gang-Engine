#include <engine/audio.hpp>
#include <engine/logger.hpp>
#include <engine/filesystem.hpp>
using namespace engine;

sound * sound_spawner::new_sound_object()
{
	for (auto &i : mSounds)
		if (!i.is_playing())
			return &i;
	mSounds.emplace_back();
	return &mSounds.back();
}

sound_spawner::sound_spawner()
{
	mMixer = nullptr;
}

void sound_spawner::spawn(std::shared_ptr<sound_file> pBuffer, float pVolume, float pPitch)
{
	engine::sound* sound_object = new_sound_object();
	if (mMixer)
		sound_object->attach_mixer(*mMixer);
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

void sound_spawner::attach_mixer(mixer& pMixer)
{
	mMixer = &pMixer;
}

void sound_spawner::detach_mixer()
{
	mMixer = nullptr;
}

inline sound::sound()
{
	mReady = false;
	mMixer = nullptr;
	mMono = true;
	mSFML_stereo_source.setRelativeToListener(true);
}

sound::~sound()
{
	if (mMixer)
		mMixer->remove(*this);
}

void sound::set_sound_resource(std::shared_ptr<sound_file> pResource)
{
	stop();

	if (mMono)
	{
		logger::info("Loading in mono");
		pResource->load_buffer();
		mSFML_mono_source.setBuffer(pResource->mSFML_buffer);
	}else
	{
		logger::info("Loading in stereo");
		if (pResource->mPack)
		{
			mSfml_stream.stream.set_pack(*pResource->mPack);
			mSfml_stream.stream.open(pResource->mSound_source);
			if (!mSfml_stream.stream.is_valid())
			{
				logger::error("Failed to load stream '" + pResource->mSound_source + "' from pack");
				mReady = false;
				return;
			}
			mSFML_stereo_source.openFromStream(mSfml_stream);
		}
		else
		{
			if (!mSFML_stereo_source.openFromFile(pResource->mSound_source))
			{
				logger::error("Failed to load stream from '" + pResource->mSound_source + "'");
				mReady = false;
				return;
			}
		}
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
	if (mMono)
		mSFML_mono_source.play();
	else
		mSFML_stereo_source.play();
}

void sound::stop()
{
	if (!mSource)
		return;
	if (mMono)
		mSFML_mono_source.stop();
	else
		mSFML_stereo_source.stop();
}

void sound::pause()
{
	if (!mSource)
		return;
	if (mMono)
		mSFML_mono_source.pause();
	else
		mSFML_stereo_source.pause();
}

void sound::set_pitch(float pPitch)
{
	mSFML_stereo_source.setPitch(pPitch);
	mSFML_mono_source.setPitch(pPitch);
}

float sound::get_pitch() const
{
	if (!mSource)
		return 0;

	// Doesn't matter which one
	return mSFML_mono_source.getPitch();
}

void sound::set_loop(bool pLoop)
{
	mSFML_stereo_source.setLoop(pLoop);
	mSFML_mono_source.setLoop(pLoop);
}

bool sound::get_loop() const
{
	if (!mSource)
		return false;

	// Doesn't matter which one
	return mSFML_mono_source.getLoop();
}

void sound::set_volume(float pVolume)
{
	float volume = util::clamp(pVolume, 0.f, 1.f);
	mVolume = volume;
	if (mMixer)
		volume *= mMixer->get_master_volume();
	mSFML_stereo_source.setVolume(volume*100);
	mSFML_mono_source.setVolume(volume*100);
}

float sound::get_volume() const
{
	return mVolume;
}

void sound::update_volume()
{
	set_volume(mVolume);
}

bool sound::is_playing() const
{
	if (!mSource)
		return false;
	if (mMono)
		return mSFML_mono_source.getStatus() == sf::SoundSource::Playing;
	else
		return mSFML_stereo_source.getStatus() == sf::SoundSource::Playing;
}
float sound::get_duration() const
{
	if (!mSource)
		return 0;
	if (mMono)
		return mSFML_mono_source.getBuffer()->getDuration().asSeconds();
	else
		return mSFML_stereo_source.getDuration().asSeconds();
}
float sound::get_playoffset() const
{
	if (!mSource)
		return 0;
	if (mMono)
		return mSFML_mono_source.getPlayingOffset().asSeconds();
	else
		return mSFML_stereo_source.getPlayingOffset().asSeconds();
}
void sound::set_playoffset(float pSeconds)
{
	if (!mSource)
		return;
	if (mMono)
		mSFML_mono_source.setPlayingOffset(sf::seconds(pSeconds));
	else
		mSFML_stereo_source.setPlayingOffset(sf::seconds(pSeconds));
}

bool sound::attach_mixer(mixer& pMixer)
{
	return pMixer.add(*this);
}

bool sound::detach_mixer()
{
	if (!mMixer)
		return false;
	return mMixer->remove(*this);
}

void sound::set_mono(bool pIs_mono)
{
	mMono = pIs_mono;

	// Setup resource
	if (mSource)
		set_sound_resource(mSource);
}

bool sound::get_mono() const
{
	return mMono;
}

inline sf::Int64 sound::sfml_stream::read(void * pData, sf::Int64 pSize)
{
	if (!stream.is_valid())
		return -1;
	return stream.read((char*)pData, pSize);
}

inline sf::Int64 sound::sfml_stream::seek(sf::Int64 pPosition)
{
	if (!stream.is_valid())
		return -1;
	return stream.seek(pPosition) ? pPosition : -1;
}

inline sf::Int64 sound::sfml_stream::tell()
{
	if (!stream.is_valid())
		return -1;
	return stream.tell();
}

inline sf::Int64 sound::sfml_stream::getSize()
{
	if (!stream.is_valid())
		return -1;
	return stream.size();
}

bool sound_file::load()
{
	bool preload = false;
	if (mPack)
	{
		pack_stream stream(*mPack, mSound_source);
		if (stream && stream.size() < streaming_threshold)
			return set_loaded(load_buffer());
	}
	else if (fs::file_size(mSound_source) < streaming_threshold)
		return set_loaded(load_buffer());
	return set_loaded(true);
}

bool sound_file::unload()
{
	mBuffer_loaded = false;
	mSFML_buffer = sf::SoundBuffer();
	set_loaded(false);
	return true;
}

void sound_file::set_filepath(const std::string & pPath)
{
	mSound_source = pPath;
}

bool sound_file::load_buffer()
{
	if (mBuffer_loaded)
		return true;

	if (mPack)
	{
		pack_stream stream(*mPack);
		stream.open(mSound_source);
		auto data = stream.read_all();
		if (data.empty())
			return false;
		mBuffer_loaded = mSFML_buffer.loadFromMemory(&data[0], data.size());
	}
	mBuffer_loaded = mSFML_buffer.loadFromFile(mSound_source);

	return mBuffer_loaded;
}

mixer::mixer()
{
	mMaster_volume = 1;
}

mixer::~mixer()
{
	for (auto i : mSounds)
		i->mMixer = nullptr;
}

void mixer::set_master_volume(float pVolume)
{
	mMaster_volume = util::clamp(pVolume, 0.f, 1.f);
	for (auto i : mSounds)
		i->update_volume();
}

float mixer::get_master_volume() const
{
	return mMaster_volume;
}

bool mixer::add(sound & pSound)
{
	for (auto i : mSounds)
		if (i == &pSound)
			return false;
	pSound.detach_mixer();
	pSound.mMixer = this;
	pSound.update_volume();
	mSounds.push_back(&pSound);
	return true;
}

bool mixer::remove(sound & pSound)
{
	for (size_t i = 0; i < mSounds.size(); i++)
	{
		if (mSounds[i] == &pSound)
		{
			mSounds.erase(mSounds.begin() + i);
			pSound.mMixer = nullptr;
			pSound.update_volume();
		}
	}
	return false;
}
