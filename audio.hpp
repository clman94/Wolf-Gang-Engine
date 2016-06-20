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
		std::list<sound> sl;
		void clean_list()
		{
			for (auto i = sl.begin(); i != sl.end(); i++)
			{
				if (!i->is_playing())
				{
					sl.erase(i);
				}
			}
		}
	public:
		void spawn(sound_buffer& buf)
		{
			sl.emplace_back(buf);
			sl.back().play();
			clean_list();
		}
		void stop_all()
		{
			for (auto &i : sl)
			{
				i.stop();
			}
			sl.clear();
		}
	};

	class sound_stream
	{
		sf::Music s;
	public:
		int open(const std::string path)
		{
			return !s.openFromFile(path);
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
		bool is_playing()
		{
			return s.getStatus() == s.Playing;
		}
	};
}