/// \weakgroup FX
/// \{

namespace fx
{
	/// \weakgroup FX
	/// \{

	/// Play sound.
	int sound(const string&in pName, float pVolume = 100, float pPitch = 1)
	{
		return _spawn_sound(pName, pVolume, pPitch);
	}
	
	/// Stop all sound effects that might be playing.
	void stop_all()
	{
		_stop_all();
	}
	
	/// Shake camera. Kind-of like a rumble effect.
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
	
	/// \}
}

/// Fade in to scene
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

/// Fade out from scene
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

/// Slowly decrease the opacity of the entity to
/// "fade" it out.
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

/// Slowly increase the opacity of the entity to
/// "fade" it in.
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

/// \}