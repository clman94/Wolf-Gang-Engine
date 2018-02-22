#ifndef RPG_SCRIPT_FUNCTION_HPP
#define RPG_SCRIPT_FUNCTION_HPP

#include <engine/utility.hpp>

#include <rpg/script_system.hpp>

namespace AS = AngelScript;

namespace rpg {

// A reference to a script function.
class script_function
{
public:
	bool is_running();
	void set_arg(unsigned int index, void* ptr);
	bool call();

private:
	util::optional_pointer<AS::asIScriptFunction> mFunction;
	util::optional_pointer<script_system>         mScript_system;
	std::shared_ptr<script_system::thread>        mFunc_ctx;
	void return_context();

	friend class collision_system; // TODO: Change this soon!
	friend class scene_script_context;
};

}
#endif // !RPG_SCRIPT_FUNCTION_HPP
