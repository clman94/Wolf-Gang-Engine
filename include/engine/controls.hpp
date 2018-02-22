#ifndef ENGINE_CONTROLS_HPP
#define ENGINE_CONTROLS_HPP

#include <map>
#include <string>
#include <vector>

#include <engine/renderer.hpp>
#include <engine/utility.hpp>

namespace engine {

class key
{
public:
	key();
	key(renderer::key_code pKey_code);
	key(const std::string& pKey_name);

	void set_key(renderer::key_code pKey_code);
	void set_key(const std::string& pKey_name);

	void set_constant(bool mIs_constant);
	bool is_constant() const;

	std::string to_string() const;
	operator std::string() const;

	renderer::key_code get_key_code() const;
private:
	renderer::key_code mKey_code;
};

typedef void(*key_binding_callback)();

class key_binding
{
public:
	bool is_triggered() const;

private:

};

class controls
{
public:
	bool is_triggered(const std::string& pName);
	void update(renderer& pR);

	bool set_press_only(const std::string& pName, bool pIs_press_only);
	bool bind_key(const std::string& pName, const std::string& pKey_name, bool pAlternative = false);
	void clear();

	bool is_enabled(const std::string& pName) const;
	bool set_enabled(const std::string& pName, bool pIs_enabled);

private:

	typedef std::vector<renderer::key_code> bound_keys;
	struct binding
	{
		binding();
		bool mIs_enabled;
		bool mIs_pressed;
		bool mPress_only;
		bound_keys keys[2]; // 2 different bindings allowed currently
	};
	std::map<std::string, binding> mBindings;
};

}
#endif // !RPG_CONTROLS_HPP
