#include <engine/controls.hpp>

#include <algorithm>
#include <array>

using namespace engine;

namespace {

const std::vector<std::pair<std::string, renderer::key_code>> key_names_codes =
{
	{ "up",     renderer::key_code::Up },
	{ "down",   renderer::key_code::Down },
	{ "left",   renderer::key_code::Left },
	{ "right",  renderer::key_code::Right },

	{ "lctrl",  renderer::key_code::LControl },
	{ "rctrl",  renderer::key_code::RControl },
	{ "lshift", renderer::key_code::LShift },
	{ "rshift", renderer::key_code::RShift },

	{ "return", renderer::key_code::Return },
	{ "tab",    renderer::key_code::Tab },

	{ "+",      renderer::key_code::Add },
	{ "-",      renderer::key_code::Subtract },
	{ "*",      renderer::key_code::Multiply },
	{ "/",      renderer::key_code::Divide },
};

}

renderer::key_code key_name_to_code(const std::string& pName)
{
	using key_code = renderer::key_code;

	std::string lc_name(pName);
	std::transform(pName.begin(), pName.end(), lc_name.begin(), ::tolower);
	lc_name.erase(std::remove_if(lc_name.begin(), lc_name.end(), ::isspace), lc_name.end());

	// a-z
	if (lc_name.size() == 1)
	{
		if (lc_name[0] >= 'a'
			&& lc_name[0] <= 'z')
			return static_cast<key_code>(key_code::A
				+ (lc_name[0] - 'a'));
	}

	// "num#"
	if (lc_name.size() == 4)
	{
		if (lc_name.substr(0, 3) == "num"
			&& lc_name[3] >= '0'
			&& lc_name[3] <= '9')
			return static_cast<key_code>(key_code::Num0
				+ (lc_name[3] - '0'));
	}

	// Search for key name
	auto find_item = std::find_if(key_names_codes.begin(), key_names_codes.end()
		, [&](const std::pair<std::string, key_code>& p)->bool { return p.first == lc_name; });
	if (find_item != key_names_codes.end())
		return find_item->second;

	return key_code::Unknown;
}

std::string key_code_to_name(renderer::key_code pCode)
{
	using key_code = renderer::key_code;
	if (pCode >= key_code::A && pCode <= key_code::Z)
		return std::string(1, (char)((pCode - key_code::A) + 'a'));

	if (pCode >= key_code::Num0 && pCode <= key_code::Num9)
		return std::string("num") + std::to_string(pCode - key_code::Num0);

	auto find_item = std::find_if(key_names_codes.begin(), key_names_codes.end()
		, [&](const std::pair<std::string, key_code>& p)->bool { return p.second == pCode; });
	if (find_item != key_names_codes.end())
		return find_item->first;
	return{};
}

bool controls::is_triggered(const std::string& pName)
{
	const auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false; // Not found
	return find->second.mIs_pressed;
}

void controls::update(renderer & pR)
{
	for (auto& i : mBindings)
	{
		if (!i.second.mIs_enabled)
		{
			i.second.mIs_pressed = false;
			continue;
		}

		for (const auto& j : i.second.keys) // Check all alternatives
		{
			if (j.empty())
				continue;

			i.second.mIs_pressed = true;
			for (const auto& k : j)
			{
				bool is_pressed = i.second.mPress_only ? pR.is_key_pressed(k) : pR.is_key_down(k);
				if (!is_pressed)
				{
					i.second.mIs_pressed = false;
					break;
				}
			}
			if (i.second.mIs_pressed)
				break;
		}
	}
}

bool controls::set_press_only(const std::string & pName, bool pIs_press_only)
{
	const auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false; // Not found
	find->second.mPress_only = pIs_press_only;
	return true;
}

bool controls::bind_key(const std::string & pName, const std::string & pKey_name, bool pAlternative)
{
	const auto key = key_name_to_code(pKey_name);
	if (key == renderer::key_code::Unknown) // Could not translate
		return false;

	const size_t alternative = pAlternative ? 1 : 0;

	// Prevent duplicates
	for (auto& i : mBindings[pName].keys[alternative])
		if (i == key)
			return false;

	mBindings[pName].keys[alternative].push_back(key);
	return true;
}

void controls::clear()
{
	mBindings.clear();
}

bool controls::is_enabled(const std::string& pName) const
{
	auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false;
	return find->second.mIs_enabled;
}

bool controls::set_enabled(const std::string & pName, bool pIs_enabled)
{
	auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false;
	find->second.mIs_enabled = pIs_enabled;
	return true;
}

controls::binding::binding()
{
	mIs_enabled = true;
	mIs_pressed = false;
	mPress_only = false;
}