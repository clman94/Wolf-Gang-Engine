
#include <rpg/flag_container.hpp>
#include <rpg/script_system.hpp>

using namespace rpg;

bool flag_container::set_flag(const std::string& pName)
{
	return mFlags.emplace(pName).second;
}
bool flag_container::unset_flag(const std::string& pName)
{
	return mFlags.erase(pName) == 1;
}
bool flag_container::has_flag(const std::string& pName) const
{
	return mFlags.find(pName) != mFlags.end();
}

void flag_container::load_script_interface(script_system & pScript)
{
	pScript.add_function("bool has_flag(const string &in)", AS::asMETHOD(flag_container, has_flag), this);
	pScript.add_function("bool set_flag(const string &in)", AS::asMETHOD(flag_container, set_flag), this);
	pScript.add_function("bool unset_flag(const string &in)", AS::asMETHOD(flag_container, unset_flag), this);
}

void flag_container::clean()
{
	mFlags.clear();
}
