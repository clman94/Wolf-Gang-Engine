#ifndef RPG_FLAG_CONTAINER_HPP
#define RPG_FLAG_CONTAINER_HPP

#include <string>
#include <set>

namespace rpg {

class script_system;

/// Contains all flags and an interface to them
class flag_container
{
public:
	bool set_flag(const std::string& pName);
	bool unset_flag(const std::string& pName);
	bool has_flag(const std::string& pName) const;
	void load_script_interface(script_system& pScript);
	void clear();


	size_t get_count() const;

	auto begin()
	{
		return mFlags.begin();
	}

	auto end()
	{
		return mFlags.end();
	}

private:
	std::set<std::string> mFlags;
};

}
#endif // !RPG_FLAG_CONTAINER_HPP
