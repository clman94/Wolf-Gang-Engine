
vec move_towards(const vec&in pTarget, const vec&in pFollower, float pSpeed = 1)
{
	return pFollower + ((pTarget - pFollower).normalize()*get_delta()*pSpeed);
}