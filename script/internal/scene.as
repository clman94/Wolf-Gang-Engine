// This file is automatically included in all (most) scripts

// Note: All of the functions with an underscore prefix are
//       not to be called by a normal script.

#include "math.as"
#include "speed.as"
#include "vector_tools.as"
#include "scoped_entity.as"
#include "narrative.as"
#include "follow_character.as"
#include "music.as"

enum anchor
{
	top = 0,
	topleft,
	topright,
	bottom,
	bottomleft,
	bottomright,
	left,
	right,
	center
};

enum control
{
	activate = 0,
	left,
	right,
	up,
	down,
	select_next,
	select_previous,
	reset,
	menu
};

enum fixed_depth
{
	overlay,
	below,
	background
};

enum direction
{	
	left = 1,
	right,
	up,
	down
};

namespace player
{
	shared void lock(bool pIs_locked)
	{
		_lockplayer(pIs_locked);
	}
	
	shared void focus(bool pIs_focus = true)
	{
		focus_player(pIs_focus);
	}
	
	shared void unfocus()
	{
		focus_player(false);
	}
}

namespace fx
{
	int sound(const string&in pName, float pVolume = 100, float pPitch = 1)
	{
		return _spawn_sound(pName, pVolume, pPitch);
	}
	
	void stop_all()
	{
		_stop_all();
	}
	
	void shake(float pSeconds, float pAmount)
	{
		
		vec original_focus = get_focus();
		
		float timer = 0;
		float shake_timer = 0;
		while (timer < pSeconds)
		{
			float delta = get_delta();
			timer += delta;
			shake_timer += delta;
			
			if (shake_timer >= 0.07)
			{
				shake_timer = 0;
				set_focus(original_focus + vec(pAmount, 0).rotate(random(0, 360)));
			}
			
			yield();
		}
		set_focus(original_focus);
	}
}

void set_anchor(entity pEntity, anchor pAnchor)
{
	_set_anchor(pEntity, pAnchor);
}

void set_depth(entity pEntity, fixed_depth pDepth)
{
	set_depth_fixed(pEntity, true);
	switch (pDepth)
	{
	case fixed_depth::overlay:
		set_depth(pEntity, -100000);
		break;
	case fixed_depth::below:
		set_depth(pEntity, 102);
		break;
	case fixed_depth::background:
		set_depth(pEntity, 100000);
		break;
	}
}

bool is_triggered(control pControls)
{
	return _is_triggered(pControls);
}

void once_flag(const string&in pFlag)
{
	if (has_flag(pFlag))
		abort();
	set_flag(pFlag);
}

void allow_loop()
{
	yield();
}

// Calculate direction based on a vector
direction vector_direction(vec pVec)
{
	if (abs(pVec.x) > abs(pVec.y))
	{
		if (pVec.x > 0)
			return direction::right;
		else
			return direction::left;
	}else{
		if (pVec.y > 0)
			return direction::down;
		else
			return direction::up;
	}
}

void set_direction(entity pEntity, vec pTowards)
{
	vec position = get_position(pEntity);
	set_direction(pEntity, vector_direction(pTowards - position));
}

// Move to a position in x seconds
void move(entity pEntity, vec pTo, float pSeconds)
{
	vec initual_position = get_position(pEntity);
	
	set_direction(pEntity, pTo);
	
	float distance = initual_position.distance(pTo);
	vec velocity = ((pTo - initual_position))/pSeconds;
	
	vec position = initual_position;
	
	float timer = 0;
	start_animation(pEntity);
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		position += velocity*delta;
		set_position(pEntity, position);
		yield();
	}
	set_position(pEntity, initual_position + (velocity*pSeconds)); // Ensure position
	stop_animation(pEntity);
}

// Move to a position at x speed
void move(entity pEntity, vec pTo, speed pSpeed)
{
	move(pEntity, pTo, pSpeed.get_time(get_position(pEntity).distance(pTo)));
}

// Move in a direction at x distance in y seconds
void move(entity pEntity, direction pDirection, float pDistance, float pSeconds)
{
	set_direction(pEntity, pDirection);
	
	vec velocity;
	
	switch(pDirection)
	{
	case direction::left:  velocity = vec(-1, 0); break;
	case direction::right: velocity = vec(1, 0);  break;
	case direction::up:    velocity = vec(0, -1); break;
	case direction::down:  velocity = vec(0, 1);  break;
	}
	
	velocity *= pDistance/pSeconds;
	
	vec initual_position = get_position(pEntity);
	vec position = initual_position;
	
	float timer = 0;
	start_animation(pEntity);
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		position += velocity*delta;
		set_position(pEntity, position);
		yield();
	}
	set_position(pEntity, initual_position + (velocity*pSeconds));  // Ensure position
	stop_animation(pEntity);
}

// Move in a direction at x distance at y speed
void move(entity pEntity, direction pDirection, float pDistance, speed pSpeed)
{
	move(pEntity, pDirection, pDistance, pSpeed.get_time(pDistance));
}

// Gradually move focus point to position in x seconds
void move_focus(vec pPosition, float pSeconds)
{
	vec focus = get_focus();
	float distance = focus.distance(pPosition);
	vec velocity = (pPosition - focus)/pSeconds;
	
	float timer = 0;
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		focus += velocity*delta;
		set_focus(focus);
		yield();
	}
	set_focus(pPosition); // Ensure position
}

void fade_in(float pSeconds = 1)
{
	const float speed = 255.f / pSeconds;
	float i = 255;
	float timer = 0;
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		i -= speed*delta;
		set_overlay_opacity(int(i));
		yield();
	}
	set_overlay_opacity(0);
}

void fade_out(float pSeconds = 1)
{
	const float speed = 255.f / pSeconds;
	float i = 0;
	float timer = 0;
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		i += speed*delta;
		set_overlay_opacity(int(i));
		yield();
	}
	set_overlay_opacity(255);
}

void fade_out(entity pEntity, float pSeconds)
{
	const float speed = 255.f / pSeconds;
	float i = 255;
	float timer = 0;
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		i -= speed*delta;
		set_color(pEntity, 255, 255, 255, int(i));
		yield();
	}
	set_color(pEntity, 255, 255, 255, 0);
}

void fade_in(entity pEntity, float pSeconds)
{
	const float speed = 255.f / pSeconds;
	float i = 0;
	float timer = 0;
	while (timer < pSeconds)
	{
		float delta = get_delta();
		timer += delta;
		i += speed*delta;
		set_color(pEntity, 255, 255, 255, int(i));
		yield();
	}
	set_color(pEntity, 255, 255, 255, 255);
}

void set_color(entity pEntity, int r, int g, int b)
{
	set_color(pEntity, r, g, b, 255);
}

void pathfind_move(entity pEntity, vec pDestination, float pSpeed, float pWait_for_player = 0)
{
	if (pWait_for_player < 0)
	{
		eprint("pWait_for_player should not be less than 0");
		return;
	}

	array<vec> path;
	if (!find_path(path, get_position(pEntity).floor(), pDestination.floor()))
	{
		eprint("Could not find path");
		return;
	}
	for (uint i = 1; i < path.length(); i++)
	{
		if (pWait_for_player != 0)
		{
			vec player_position = get_position(get_player());
			vec position = get_position(pEntity);
			
			while (get_position(get_player()).distance(position) >= pWait_for_player)
			{
				set_direction(pEntity, player_position);
				yield();
			}
		}
		dprint(formatFloat(path[i].x) + ", " +  formatFloat(path[i].y));
		move(pEntity, path[i] + vec(0.5f, 0.5f), speed(pSpeed));
	}
}
