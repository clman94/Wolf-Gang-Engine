/// \weakgroup Narrative
/// \{

/// A return from a 2 choice selection.
/// \see select
enum option
{
	first,
	second
};

/// \}

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
	
	/// \addtogroup Narrative
	/// \{
	
	/// Set the sound effect for reveal text.
	void set_dialog_sound(string &in pName)
	{
		narrative::priv::current_dialog_sound = pName;
	}
	
	/// Add an entity whose animation will play when dialogue is appearing.
	void add_speaker(entity pEntity)
	{
		narrative::priv::speakers.insertLast(pEntity);
	}
	
	/// Remove all entities that are to "speak"
	void clear_speakers()
	{
		uint size = narrative::priv::speakers.length();
		narrative::priv::speakers.removeRange(0, size);
	}
	
	/// Make the narrative show/reappear.
	void show()
	{
		player::lock(true);
		if (!_is_box_open())
			_showbox();
	}
	
	/// Hide the dialog box. The current session is not closed
	/// and all settings are kept.
	/// \see narrative::show
	void hide()
	{
		_hidebox();
	}
	
	/// End the dialog session.
	/// A dialog session consists of changes
	/// made to the narrative box and speakers added.
	void end(bool pUnlock_player = true)
	{
		clear_speakers();
		player::lock(!pUnlock_player);
		narrative::priv::current_dialog_sound = "dialog_sound";
		narrative::priv::randomized_dialog_sound = false;
		_end_narrative();
	}
	
	/// Position to place the narrative box.
	enum box
	{
		top = 0,
		bottom
	};
	
	/// Changes the position of the dialogue box
	void move_box(box pPosition)
	{
		_set_box_position(int(pPosition));
	}
	
	/// Get entity referencing the box of the narrative box.
	entity get_box()
	{
		return _get_narrative_box();
	}
	
	/// Set the interval between each character
	void set_interval(float ms)
	{
		_set_interval(ms);
	}
	
	/// Set interval between each character based on characters per second
	void set_speed(float pSpeed)
	{
		set_interval(1000.f/pSpeed);
	}
	
	/// Set the expression of be shown in the left of the dialog box
	void set_expression(const string&in pName)
	{
		_set_expression(pName);
	}
	
	/// \}
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

/// \addtogroup Narrative
/// \{

/// Wait for key before continuing.
void keywait(int pControl = control::activate)
{
	do { yield(); }
	while (!_is_triggered(pControl));
}

/// Open and reveal text without waiting for key to continue.
void fsay(const string&in msg)
{
	narrative::show();
	_say(msg, false);
	_wait_dialog_reveal();
}

/// Open and reveal text and wait for key to continue.
void say(const string&in msg)
{
	fsay(msg);
	keywait();
}

/// Append text to the narrative without waiting for key.
/// The dialogue is required to be open for this to work properly.
void fappend(const string&in msg)
{
	_say(msg, true);
	_wait_dialog_reveal();
}

/// Append text to the narrative and wait for key.
/// The dialogue is required to be open for this to work properly.
void append(const string&in msg)
{
	fappend(msg);
	keywait();
}

/// Append and newline of text to the narrative without waiting for key.
/// The dialogue is required to be open for this to work properly.
void fnewline(const string&in msg)
{
	fappend("\n" + msg);
}

/// Append and newline of text to the narrative and wait for key.
/// The dialogue is required to be open for this to work properly.
void newline(const string&in msg)
{
	append("\n" + msg);
}

/// Append and newline of text to the narrative without waiting for key.
/// The dialogue is required to be open for this to work properly.
/// Short-cut for fnewline.
/// \see nl
void fnl(const string&in msg)
{
	fnewline(msg);
}

/// Append and newline of text to the narrative and wait for key.
/// The dialogue is required to be open for this to work properly.
/// Short-cut for newline.
void nl(const string&in msg)
{
	newline(msg);
}

/// Opens the selection textbox and allows user to choose between unlimited options.
/// NOTE: This might change very soon.
/// \see select
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

/// Opens the selection textbox and allows user to choose between 2 options.
/// NOTE: This might change very soon.
/// \see multiselect
option select(const string&in pOption1, const string&in pOption2)
{
	option val = option::first;
	_show_selection();
	_set_selection("*" + pOption1 + "   " + pOption2);
	do {
		if (_is_triggered(control::select_next))
		{
			val = option::second;
			_set_selection(" " + pOption1 + "  *" + pOption2);
		}
		if (_is_triggered(control::select_previous))
		{
			val = option::first;
			_set_selection("*" + pOption1 + "   " + pOption2);
		}
		yield();
	} while (!_is_triggered(control::activate));
	_hide_selection();
	return val;
}

/// Wait for a specific amount of seconds.
void wait(float pSeconds)
{
	_timer_start(pSeconds);
	while (!_timer_reached())
	{ yield(); }
}

/// \}

