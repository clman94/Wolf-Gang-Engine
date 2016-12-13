/// \weakgroup Entity
/// \{


/// Convert pixel coordinate to in-game coordinates
vec pixel(float pX = 0, float pY = 0)
{
	return vec(pX, pY)/32;
}

/// Convert pixel coordinate to in-game coordinates
vec pixel(vec pVec)
{
	return pVec/32;
}

/// \}

vec move_towards(const vec&in pTarget, const vec&in pFollower, float pSpeed = 1)
{
	return pFollower + ((pTarget - pFollower).normalize()*get_delta()*pSpeed);
}