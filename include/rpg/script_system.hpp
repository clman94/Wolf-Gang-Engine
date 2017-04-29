#ifndef RPG_SCRIPT_SYSTEM_HPP
#define RPG_SCRIPT_SYSTEM_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>


#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <add_on/scriptbuilder/scriptbuilder.h>
#include <add_on/scriptarray/scriptarray.h>
#include <add_on/scripthandle/scripthandle.h>
#include <add_on/scriptdictionary/scriptdictionary.h>

#include <memory>
#include <list>

namespace AS = AngelScript;

namespace rpg
{


class script_system
{
public:
	script_system();
	~script_system();

	// Register a member function, will require the pointer to the instance
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);

	// Register a non-member/static function
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);

	void abort_all();
	void return_context(AS::asIScriptContext* pContext);
	int tick();
	int get_current_line();

	void set_namespace(const std::string& pName);
	void reset_namespace();

	AS::asIScriptEngine& get_engine();

	AS::asIScriptContext* create_thread(AS::asIScriptFunction *pFunc, bool keep_context = false);

	bool is_executing();

	template<typename T>
	static void script_default_constructor(void *pMemory)
	{
		new(pMemory) T();
	}

	template<typename T, typename Targ1>
	static void script_constructor(Targ1 pArg1, void *pMemory)
	{
		new(pMemory) T(pArg1);
	}

	template<typename T, typename Targ1, typename Targ2>
	static void script_constructor(Targ1 pArg1, Targ2 pArg2, void *pMemory)
	{
		new(pMemory) T(pArg1, pArg2);
	}

	template<typename T>
	static void script_default_deconstructor(void *pMemory)
	{
		((T*)pMemory)->~T();
	}

private:

	struct thread
	{
		AS::asIScriptContext* context;
		bool keep_context;
	};

	thread* mCurrect_thread_context;
	std::vector<std::unique_ptr<thread>> mThread_contexts;

	util::optional_pointer<AS::asIScriptEngine> mEngine;

	void register_vector_type();
	void register_timer_type();
	void message_callback(const AS::asSMessageInfo * msg);

	void script_abort();
	void script_debug_print(std::string &pMessage);
	void script_error_print(std::string &pMessage);
	void script_create_thread(AS::asIScriptFunction *func, AS::CScriptDictionary *arg);
	void script_create_thread_noargs(AS::asIScriptFunction *func);
	bool script_yield();

	std::map<std::string, AS::CScriptHandle> mShared_handles;

	void script_make_shared(AS::CScriptHandle pHandle, const std::string& pName);
	AS::CScriptHandle script_get_shared(const std::string& pName);

	void load_script_interface();
};



}
#endif // !RPG_SCRIPT_SYSTEM_HPP
