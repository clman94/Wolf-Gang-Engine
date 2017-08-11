#ifndef RPG_SCRIPT_SYSTEM_HPP
#define RPG_SCRIPT_SYSTEM_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>
#include <engine/vector.hpp>


#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/scriptbuilder/scriptbuilder.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include <angelscript/add_on/scripthandle/scripthandle.h>
#include <angelscript/add_on/scriptdictionary/scriptdictionary.h>

#include <memory>
#include <list>

#include <engine/AS_utility.hpp>

namespace AS = AngelScript;

template<typename T>
struct AS_array : public AS::CScriptArray {};

namespace util {
template<>
struct AS_type_to_string<engine::fvector> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "vec";
	}
};

template<typename T>
struct AS_type_to_string<AS_array<T>> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "array<" + AS_type_to_string<T>().string() + ">";
	}
};
}

namespace rpg
{




class script_system
{
public:

	struct thread
	{
		AS::asIScriptContext* context;
		bool keep_context;
	};

	script_system();
	~script_system();



	template<typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret (* mFunction)(Tparams...))
	{
		const std::string declaration = util::AS_create_function_declaration<Tret, Tparams...>(pName);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asFUNCTION(mFunction)
			, AS::asCALL_CDECL);
		assert(r >= 0);
	}

	// Method global function binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret(Tclass::*mFunction)(Tparams...), void* pInstance)
	{
		const std::string declaration = util::AS_create_function_declaration<Tret, Tparams...>(pName);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL_ASGLOBAL, pInstance);
		assert(r >= 0);
	}

	// Const method global function binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret(Tclass::*mFunction)(Tparams...) const, void* pInstance)
	{
		const std::string declaration = util::AS_create_function_declaration<Tret, Tparams...>(pName);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL_ASGLOBAL, pInstance);
		assert(r >= 0);
	}

	template<typename T>
	void create_object(const std::string& pName)
	{
		mEngine->RegisterObjectType(pName.c_str()
			, sizeof(T)
			, AS::asOBJ_VALUE | AS::asGetTypeTraits<T>());
		mEngine->RegisterObjectBehaviour(pName.c_str(), AS::asBEHAVE_CONSTRUCT, "void f()"
			, AS::asFUNCTION(script_system::script_default_constructor<T>)
			, AS::asCALL_CDECL_OBJLAST);
		mEngine->RegisterObjectBehaviour(pName.c_str(), AS::asBEHAVE_DESTRUCT, "void f()"
			, AS::asFUNCTION(script_system::script_default_deconstructor<T>)
			, AS::asCALL_CDECL_OBJLAST);
	}

	template<typename Tclass, typename Tret, typename...Tparams>
	void add_method(const std::string& pObject, const std::string& pName, Tret(Tclass::*mFunction)(Tparams...))
	{
		std::string declaration = util::AS_create_function_declaration<Tret, Tparams...>(pName);

		mEngine->RegisterObjectMethod(pObject.c_str(), declaration.c_str()
			, AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL);
	}

	// Const method object binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_method(const std::string& pObject, const std::string& pName, Tret(Tclass::*mFunction)(Tparams...) const)
	{
		const std::string declaration = util::AS_create_function_declaration<Tret, Tparams...>(pName) + " const";

		mEngine->RegisterObjectMethod(pObject.c_str(), declaration.c_str()
			, AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL);
	}

	void abort_all();
	void return_context(AS::asIScriptContext* pContext);
	int tick();
	int get_current_line();

	void set_namespace(const std::string& pName);
	void reset_namespace();

	AS::asIScriptEngine& get_engine();

	std::shared_ptr<thread> create_thread(AS::asIScriptFunction *pFunc, bool keep_context = false);

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

	// These are kept for the "special" functions
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);

	engine::timer mTimeout_timer;

	std::shared_ptr<thread> mCurrect_thread_context;
	std::vector<std::shared_ptr<thread>> mThread_contexts;

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

	void timeout_callback(AS::asIScriptContext *ctx);
};

}



#endif // !RPG_SCRIPT_SYSTEM_HPP
