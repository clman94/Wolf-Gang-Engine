

void on_update(float)
{
	if (gameInput.down(key::w))
		dprint("w");
	if (gameInput.down(key::a))
		dprint("a");
	if (gameInput.down(key::s))
		dprint("s");
	if (gameInput.down(key::d))
		dprint("d");
	if (gameInput.pressed(key::space))
		dprint("Space");
}
/*

function on_update(delta float)
{
	if (gameInput.down(key::w))
		dprint("w");
	if (gameInput.down(key::a))
		dprint("a");
	if (gameInput.down(key::s))
		dprint("s");
	if (gameInput.down(key::d))
		dprint("d");
	if (gameInput.pressed(key::space))
		dprint("Space");
}

/*