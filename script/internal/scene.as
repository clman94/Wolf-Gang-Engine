#include "math.as"
#include "speed.as"
#include "vector_tools.as"
#include "scoped_entity.as"
#include "narrative.as"
#include "follow_character.as"
#include "music.as"
#include "fx.as"
#include "entity.as"

/// \weakgroup Game
/// \{

/// Basic control that is supported in the engine.
/// There are 2 different types of controls: pressed and held.
/// Pressed controls are only activated once in only one frame and
/// in any other frame (even if it's still being held) it will not be considered
/// activated. Held controls are simply always activated when the key is down.
/// \see is_triggered
enum control
{
	activate = 0,     ///< Typically the enter and Z key (Pressed)
	left,             ///< (Held)
	right,            ///< (Held)
	up,               ///< (Held)
	down,             ///< (Held)
	select_next,      ///< Typically the right key (Pressed)
	select_previous,  ///< Typically the left key (Pressed)
	select_up,        ///< Typically the up key (Pressed)
	select_down,      ///< Typically the down key (Pressed)
	back,             ///< X key, go back or exit (Pressed)
	menu = 11,
};

namespace player
{
	/// \weakgroup Scene
	/// \{

	/// Set whether or not the player character will receive
	/// movement events (left, right, etc). When locked, the player will
	/// simply be unable to move.
	void lock(bool pIs_locked)
	{
		_lockplayer(pIs_locked);
	}
	
	/// Set the focus of the camera to either focus on the player
	/// or freely move around. When function like set_focus are used,
	/// the focus on the player is automatically removed.
	///
	/// This function can be called without any parameters to focus on player.
	void focus(bool pIs_focus = true)
	{
		focus_player(pIs_focus);
	}
	
	/// Unfocus player.
	void unfocus()
	{
		focus_player(false);
	}
	
	/// \}
}

/// Check if a control as been activated
bool is_triggered(control pControls)
{
	return _is_triggered(pControls);
}

/// Exit thread if flag exist otherwise create the flag and continue.
/// This is useful in situations when you want a trigger to be activated once.
/// \param pFlag Keyboard smash if you have no farther use for it
void once_flag(const string&in pFlag)
{
	if (has_flag(pFlag))
		abort();
	set_flag(pFlag);
}

/// \}
