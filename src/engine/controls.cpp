#include <engine/controls.hpp>

using namespace engine;


inline util::optional<engine::renderer::key_type> translate_key_name(const std::string& pName)
{
	using key_type = engine::renderer::key_type;

	// a-z
	if (pName.size() == 1)
	{
		if (pName[0] >= 'a'
			&& pName[0] <= 'z')
			return static_cast<key_type>(
				static_cast<size_t>(key_type::A)
				+ (pName[0] - 'a'));
	}

	// "num#"
	if (pName.size() == 4)
	{
		if (pName.substr(0, 3) == "num"
			&& pName[3] >= '0'
			&& pName[3] <= '9')
			return static_cast<key_type>(
				static_cast<size_t>(key_type::Num0)
				+ (pName[3] - '0'));
	}

	// Other stuff
	const std::map<std::string, key_type> translations =
	{
		{ "up", key_type::Up },
		{ "down", key_type::Down },
		{ "left", key_type::Left },
		{ "right", key_type::Right },

		{ "lctrl", key_type::LControl },
		{ "rctrl", key_type::RControl },
		{ "lshift", key_type::LShift },
		{ "rshift", key_type::RShift },

		{ "return", key_type::Return },
		{ "tab", key_type::Tab },

		{ "+", key_type::Add },
		{ "-", key_type::Subtract },
		{ "*", key_type::Multiply },
		{ "/", key_type::Divide },
	};

	const auto find = translations.find(pName);
	if (find != translations.end())
		return find->second;

	return{};
}

bool controls::is_triggered(const std::string& pName)
{
	const auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false; // Not found
	return find->second.is_pressed;
}

void controls::update(engine::renderer & pR)
{
	for (auto& i : mBindings)
	{
		for (const auto& j : i.second.keys) // Check all alternatives
		{
			if (j.empty())
				continue;

			i.second.is_pressed = true;
			for (const auto& k : j)
			{
				bool is_pressed = i.second.press_only ? pR.is_key_pressed(k) : pR.is_key_down(k);
				if (!is_pressed)
				{
					i.second.is_pressed = false;
					break;
				}
			}
			if (i.second.is_pressed)
				break;
		}
	}
}

bool controls::set_press_only(const std::string & pName, bool pIs_press_only)
{
	const auto find = mBindings.find(pName);
	if (find == mBindings.end())
		return false; // Not found
	find->second.press_only = pIs_press_only;
	return true;
}

bool controls::bind_key(const std::string & pName, const std::string & pKey_name, bool pAlternative)
{
	const auto key = translate_key_name(pKey_name);
	if (!key) // Could not translate
		return false;

	const size_t alternative = pAlternative ? 1 : 0;

	// Prevent duplicates
	for (auto& i : mBindings[pName].keys[alternative])
		if (i == *key)
			return false;

	mBindings[pName].keys[alternative].push_back(*key);
	return true;
}

void controls::clean()
{
	mBindings.clear();
}

controls::binding::binding()
{
	is_pressed = false;
	press_only = false;
}