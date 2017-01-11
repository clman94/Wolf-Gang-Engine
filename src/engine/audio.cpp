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
