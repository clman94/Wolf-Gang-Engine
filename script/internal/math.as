// Just some extra math functions

int random(int min, int max)
{
	return (rand()%(max - min)) + min;
}

int abs(int val)
{
	return val < 0 ? -val : val;
}
