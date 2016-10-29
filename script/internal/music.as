namespace music
{

	// Set the loop
	void loop(bool pLoop)
	{
		_music_set_loop(pLoop);
	}
	
	// Stop the song
	void stop()
	{
		_music_stop();
	}
	
	// Pause the song
	void pause()
	{
		_music_pause();
	}
	
	// Is the song playing?
	bool playing()
	{
		return _music_is_playing();
	}
	
	// Play song
	void play()
	{
		if (!playing())
			_music_play();
	}
	
	// Set volume 0-100
	void volume(float pVolume)
	{
		_music_volume(pVolume);
	}
	
	// Get duration of song in seconds
	float duration()
	{
		return _music_get_duration();
	}
	
	// Get position in seconds
	float position()
	{
		return _music_position();
	}
	
	// Open sound file
	void open(const string&in pPath, bool pQuick = true)
	{
		if (_music_open(pPath) != 0)
			dprint("Could not load music file '" + pPath + "'");
		
		if (pQuick)
		{
			loop(true);
			play();
		}
	}
	
	// wait until a specific time in the song
	void wait_until(float pSeconds)
	{
		if (!music::playing())
		{
			dprint("No music is playing to wait for...");
			return;
		}
		
		if (music::duration() < pSeconds)
		{
			dprint("The duration of the music is less then the time you requested to wait");
			return;
		}
		
		while (music::playing()
		&&     music::position() < pSeconds)
		{ yield(); }
	}
}