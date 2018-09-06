
class aclass
{
	void func(int v, classofthing@ r1)
	{
		print(formatInt(v) + r1.msg);
		r1.msg = "moreyey";
	}
};


interface behavior_interface
{
	
};

class mybehavior
{

};

class classofthing
{
	string msg;
}

void start()
{

	classofthing p1;
	p1.msg = "yey";
	aclass obj;
	
	
	varfunc(obj, "void func(int, classofthing@)", 23, @p1);
	
	print(p1.msg);
}