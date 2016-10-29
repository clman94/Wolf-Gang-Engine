class speed
{
	speed()
	{}
	
	speed(float pSpeed)
	{
		mSpeed = pSpeed;
	}
	
	float get_speed() const
	{
		return mSpeed;
	}
	
	void set_speed(float pSpeed)
	{
		mSpeed = pSpeed;
	}
	
	// Calculate amount of time to move
	//  a distance at the current speed
	float get_time(float pDistance) const
	{
		return pDistance/mSpeed;
	}
	
	private float mSpeed;
};
