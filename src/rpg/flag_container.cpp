
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
	pScript.add_function("has_flag", &flag_container::has_flag, this);
	pScript.add_function("set_flag", &flag_container::set_flag, this);
	pScript.add_function("unset_flag", &flag_container::unset_flag, this);
}

void flag_container::clear()
{
	mFlags.clear();
}

size_t flag_container::get_count() const
{
	return mFlags.size();
}
