
namespace priv{

void do_character_follow(dictionary@ args)
{
	follow_character@ pCharacter = cast<follow_character>(args["pCharacter"]);
	
	array<vec> path;
	while (pCharacter.is_following())
	{
		if (get_position(pCharacter).manhattan(get_position(get_player())) <= 1)
		{
			yield();
			continue;
		}
		
		find_path_partial(path, get_position(pCharacter).floor(), get_position(get_player()).floor(), 5);
		
		
		//dprint(formatFloat(path[i].x) + " " +  formatFloat(path[i].y));
		//move(guy, path[i] + vec(random(0, 100)*0.01, random(0, 100)*0.01), speed(3));
		
		if (path.length() >= 2)
			move(pCharacter, path[1] + vec(0.5, 0.5), speed(pCharacter.get_speed()));
		
		path.resize(0);
		yield();
	}
}

}

class follow_character
{
	follow_character()
	{
		mIs_following = false;
		mSpeed = 1;
	}
	
	~follow_character()
	{
		stop();
	}

	follow_character& opAssign(entity pEntity)
	{
		mEntity	= pEntity;
		return this;
	}
	
	entity opImplConv()
	{
		return mEntity;
	}
	
	void start()
	{
		mIs_following = true;
		create_thread(priv::do_character_follow,
			dictionary = {{"pCharacter", @this}}
		);
	}
	
	void stop()
	{
		mIs_following = false;
	}
	
	void set_speed(float pSpeed)
	{
		mSpeed = pSpeed;
	}
	
	float get_speed()
	{
		return mSpeed;
	}
	
	bool is_following()
	{
		return mIs_following;
	}
	
	bool check()
	{
		array<vec> path;
		return find_path(path, get_position(mEntity).floor(), get_position(get_player()).floor());
	}
	
	private bool mIs_following;
	private entity mEntity;
	private float mSpeed;
}