#ifndef RPG_SCRIPT_FUNCTION_HPP
#define RPG_SCRIPT_FUNCTION_HPP

#include <engine/utility.hpp>

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/contextmgr/contextmgr.h>
namespace AS = AngelScript;

namespace rpg {

// A reference to a script function.
class script_function
{
public:
	script_function();
	~script_function();
	bool is_running();
	void set_engine(AS::asIScriptEngine * e);
	void set_function(AS::asIScriptFunction * f);
	void set_context_manager(AS::CContextMgr * cm);
	void set_arg(unsigned int index, void* ptr);
	bool call();

private:
	util::optional_pointer<AS::asIScriptEngine> as_engine;
	util::optional_pointer<AS::asIScriptFunction> func;
	util::optional_pointer<AS::CContextMgr> ctx;
	util::optional_pointer<AS::asIScriptContext> func_ctx;
	void return_context();
};

}
#endif // !RPG_SCRIPT_FUNCTION_HPP
