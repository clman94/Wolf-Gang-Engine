
namespace priv{

void do_character_follow(dictionary@ args)
{
	follow_character@ pCharacter = cast<follow_character>(args["pCharacter"]);
	
	array<vec> path;
	while (true)
	{
		if (get_position(pCharacter).manhattan(get_position(get_player())) <= 2)
		{
			yield();
			continue;
		}
		
		find_path_partial(path, get_position(pCharacter).floor(), get_position(get_player()).floor(), 1);
		
		for (uint i = 1; i < path.length(); i++)
		{
			//dprint(formatFloat(path[i].x) + " " +  formatFloat(path[i].y));
			//move(guy, path[i] + vec(random(0, 100)*0.01, random(0, 100)*0.01), speed(3));
			move(pCharacter, path[i] + vec(0.5, 0.5), speed(1));
		}
		
		if (!pCharacter.is_following())
			return;
		
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
			dictionary = {{"pCharacter", this}}
		);
	}
	
	void stop()
	{
		mIs_following = false;
	}
	
	bool is_following()
	{
		return mIs_following;
	}
	
	private bool mIs_following;
	private entity mEntity;
}