
enum option
{
	first = 0,
	second
};

namespace narrative
{
namespace priv
{	
	array<entity> speakers;
	void validate_speaker(uint i)
	{
		if (!speakers[i].is_valid())
			speakers.removeAt(i);
	}
	void start_speakers()
	{
		for (uint i = 0; i < speakers.length(); i++)
			start_animation(speakers[i]);
	}
	
	void stop_speakers()
	{
		for (uint i = 0; i < speakers.length(); i++)
			stop_animation(speakers[i]);
	}
	
	
	string current_dialog_sound = "dialog_sound";
	bool randomized_dialog_sound = false;
}
	
	// Set the sound to activate each character
	void set_dialog_sound(string &in pName)
	{
		narrative::priv::current_dialog_sound = pName;
	}
	
	// Add an entity whose animation will play when dialog is appearing
	void add_speaker(entity pEntity)
	{
		narrative::priv::speakers.insertLast(pEntity);
	}
	
	// Remove all entities that are to speak
	void clear_speakers()
	{
		uint size = narrative::priv::speakers.length();
		narrative::priv::speakers.removeRange(0, size);
	}
	
	// Reappear the dialog box
	void show()
	{
		player::lock(true);
		_showbox();
	}
	
	// Hide the dialog box. Use show() to make it appear again
	// Use end() when you want to "end" the dialog session
	void hide()
	{
		_hidebox();
	}
	
	// End the dialog session
	void end(bool pUnlock_player = true)
	{
		clear_speakers();
		player::lock(!pUnlock_player);
		narrative::priv::current_dialog_sound = "dialog_sound";
		narrative::priv::randomized_dialog_sound = false;
		_end_narrative();
	}
	
	enum box
	{
		top = 0,
		bottom
	};
	
	// Changes the position of the dialog box
	void move_box(int pos)
	{
		_set_box_position(pos);
	}
	
	// Set the interval between each character
	void set_interval(float ms)
	{
		_set_interval(ms);
	}
	
	// Set interval between each character based on characters per second
	void set_speed(float pSpeed)
	{
		set_interval(1000.f/pSpeed);
	}
	
	// Set the expression of be shown in the left of the dialog box
	void set_expression(const string&in pName)
	{
		_set_expression(pName);
	}
}

void _wait_dialog_reveal(bool pSkip = false)
{
	narrative::priv::start_speakers();
	_start_expression_animation();
	do {
		yield();
		if (_has_displayed_new_character())
		{
			if (narrative::priv::randomized_dialog_sound)
				fx::sound(narrative::priv::current_dialog_sound, 100, random(80, 110)*0.01);
			else
				fx::sound(narrative::priv::current_dialog_sound);
		}
		
		if (is_triggered(control::activate))
			_skip_reveal();
	} while (_is_revealing());
	_stop_expression_animation();
	narrative::priv::stop_speakers();
}

void keywait(int pControl = control::activate)
{
	do { yield(); }
	while (!_is_triggered(pControl));
}

void fsay(const string&in msg)
{
	narrative::show();
	_say(msg, false);
	_wait_dialog_reveal();
}

void say(const string&in msg)
{
	fsay(msg);
	keywait();
}

void fappend(const string&in msg)
{
	_say(msg, true);
	_wait_dialog_reveal();
}

void append(const string&in msg)
{
	fappend(msg);
	keywait();
}

void fnewline(const string&in msg)
{
	fappend("\n" + msg);
}

void newline(const string&in msg)
{
	append("\n" + msg);
}

// lazy newline functions
void fnl(const string&in msg)
{
	fnewline(msg);
}

void nl(const string&in msg)
{
	newline(msg);
}

int multiselect(const array<string>&in pSelections)
{
	int val = 0;
	_show_selection();
	_set_selection(pSelections[val]);
	do {
		if (_is_triggered(control::select_next))
		{
			++val;
			val %= pSelections.length();
			_set_selection(pSelections[val]);
		}
		if (_is_triggered(control::select_previous))
		{
			--val;
			val = abs(val);
			val %= pSelections.length();
			_set_selection(pSelections[val]);
		}
		yield();
	} while (!_is_triggered(control::activate));
	_hide_selection();
	return val;
}

option select(const string&in a, const string&in b)
{
	option val = option::first;
	_show_selection();
	_set_selection("*" + a + "   " + b);
	do {
		if (_is_triggered(control::select_next))
		{
			val = option::second;
			_set_selection(" " + a + "  *" + b);
		}
		if (_is_triggered(control::select_previous))
		{
			val = option::first;
			_set_selection("*" + a + "   " + b);
		}
		yield();
	} while (!_is_triggered(control::activate));
	_hide_selection();
	return val;
}

void wait(float pSeconds)
{
	_timer_start(pSeconds);
	while (!_timer_reached())
	{ yield(); }
}

void nwait(float pSeconds)
{
	narrative::priv::start_speakers();
	_start_expression_animation();
	wait(pSeconds);
	_stop_expression_animation();
	narrative::priv::stop_speakers();
}

