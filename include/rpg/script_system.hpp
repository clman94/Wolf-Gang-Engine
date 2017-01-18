#ifndef RPG_SCRIPT_SYSTEM_HPP
#define RPG_SCRIPT_SYSTEM_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/contextmgr/contextmgr.h>
#include <angelscript/add_on/scriptbuilder/scriptbuilder.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include <angelscript/add_on/scripthandle/scripthandle.h>

#include <rpg/collision_box.hpp>


namespace AS = AngelScript;

namespace rpg
{

class script_context;

class script_system
{
public:
	script_system();
	~script_system();

	void load_context(script_context& pContext);

	// Register a member function, will require the pointer to the instance
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);

	// Register a non-member/static function
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);

	void abort_all();

	// Call all functions that contain the specific metadata
	void start_all_with_tag(const std::string& pTag);

	// Execute all scripts
	int tick();

	int get_current_line();

	AS::asIScriptEngine& get_engine();

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
	util::optional_pointer<AS::asIScriptEngine> mEngine;
	AS::CContextMgr      mCtxmgr;
	util::optional_pointer<script_context> mContext;
	std::ofstream        mLog_file;
	bool                 mExecuting;

	engine::timer mTimer;



	void register_vector_type();
	void message_callback(const AS::asSMessageInfo * msg);

	void script_abort();
	void script_debug_print(std::string &pMessage);
	void script_error_print(std::string &pMessage);
	void script_create_thread(AS::asIScriptFunction *func, AS::CScriptDictionary *arg);
	void script_create_thread_noargs(AS::asIScriptFunction *func);

	std::map<std::string, AS::CScriptHandle> mShared_handles;

	void script_make_shared(AS::CScriptHandle pHandle, const std::string& pName);
	AS::CScriptHandle script_get_shared(const std::string& pName);

	void load_script_interface();

	friend class script_context;
};


class script_context
{
public:
	script_context();
	~script_context();

	void set_script_system(script_system& pScript);
	bool build_script(const std::string& pPath);
	bool is_valid() const;
	void clean();

	const std::vector<trigger>& get_script_defined_triggers() const;
	const std::vector<trigger>& get_script_defined_buttons() const;

private:

	// Constructs all triggers/buttons defined by functions
	// with metadata "trigger" and "button"
	// TODO: Stablize parsing of metadata
	void parse_script_defined_triggers();

	util::optional_pointer<script_system> mScript;
	util::optional_pointer<AS::asIScriptModule> mScene_module;
	AS::CScriptBuilder mBuilder;

	std::vector<trigger> mScript_defined_triggers;
	std::vector<trigger> mScript_defined_buttons;

	std::map<std::string, std::shared_ptr<script_function>> mTrigger_functions;

	static std::string get_metadata_type(const std::string &pMetadata);

	friend class script_system;
};

}
#endif // !RPG_SCRIPT_SYSTEM_HPP
