#ifndef RPG_SCRIPT_SYSTEM_HPP
#define RPG_SCRIPT_SYSTEM_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>
#include <engine/vector.hpp>
#include <engine/logger.hpp>

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include "../3rdparty/AngelScript/sdk/add_on/scriptbuilder/scriptbuilder.h"
#include "../3rdparty/AngelScript/sdk/add_on/scriptarray/scriptarray.h"
#include "../3rdparty/AngelScript/sdk/add_on/scripthandle/scripthandle.h"
#include "../3rdparty/AngelScript/sdk/add_on/scriptdictionary/scriptdictionary.h"

#include <memory>
#include <list>

#include <engine/AS_utility.hpp>

namespace AS = AngelScript;

// This allows us to separate the arrays with different types
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

namespace operator_method
{
const std::string assign = "opAssign";
const std::string additive_assign = "opAddAssign";
const std::string subtractive_assign = "opSubAssign";
const std::string multiplicative_assign = "opMulAssign";
const std::string dividing_assign = "opDivAssign";

const std::string add = "opAdd";
const std::string subtract = "opSub";
const std::string multiply = "opMul";
const std::string divide = "opDiv";

const std::string negative = "opNeg";

const std::string equals = "opEquals";
}

class script_system
{
public:

	script_system();
	~script_system();

	template<typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret (* mFunction)(Tparams...))
	{
		const std::string declaration = util::AS_create_function_declaration(pName, mFunction);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asFUNCTION(mFunction)
			, AS::asCALL_CDECL);
		assert(r >= 0);
	}

	// Method global function binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret(Tclass::*mFunction)(Tparams...), void* pInstance)
	{
		Tret(*of_new_type)(Tparams...) = nullptr; // gcc complains about ambiguities when directly
		                                          // specifing the types in util::AS_create_function_declaration
		const std::string declaration = util::AS_create_function_declaration(pName, of_new_type);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL_ASGLOBAL, pInstance);
		assert(r >= 0);
	}

	// Const method global function binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_function(const std::string& pName, Tret(Tclass::*mFunction)(Tparams...) const, void* pInstance)
	{
		Tret(*of_new_type)(Tparams...) = nullptr;
		const std::string declaration = util::AS_create_function_declaration(pName, of_new_type);
		const int r = mEngine->RegisterGlobalFunction(declaration.c_str(), AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL_ASGLOBAL, pInstance);
		assert(r >= 0);
	}

	template<typename T>
	void add_object(const std::string& pName, bool pAll_floats = false)
	{
		const int r = mEngine->RegisterObjectType(pName.c_str()
			, sizeof(T)
			, AS::asOBJ_VALUE | AS::asGetTypeTraits<T>() | (pAll_floats ? AS::asOBJ_APP_CLASS_ALLFLOATS : 0));
		assert(r >= 0);
		
		const int r1 = mEngine->RegisterObjectBehaviour(pName.c_str(), AS::asBEHAVE_CONSTRUCT, "void f()"
			, AS::asFUNCTION(script_system::script_default_constructor<T>)
			, AS::asCALL_CDECL_OBJLAST);
		assert(r1 >= 0);

		const int r2 = mEngine->RegisterObjectBehaviour(pName.c_str(), AS::asBEHAVE_DESTRUCT, "void f()"
			, AS::asFUNCTION(script_system::script_default_deconstructor<T>)
			, AS::asCALL_CDECL_OBJLAST);
		assert(r2 >= 0);
	}

	template<typename Tclass, typename Tret, typename...Tparams>
	void add_method(const std::string& pObject, const std::string& pName, Tret(Tclass::*mFunction)(Tparams...))
	{
		Tret(*of_new_type)(Tparams...) = nullptr;
		const std::string declaration = util::AS_create_function_declaration(pName, of_new_type);

		const int r = mEngine->RegisterObjectMethod(pObject.c_str(), declaration.c_str()
			, AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL);
		assert(r >= 0);
	}

	// Const method object binding
	template<typename Tclass, typename Tret, typename...Tparams>
	void add_method(const std::string& pObject, const std::string& pName, Tret(Tclass::*mFunction)(Tparams...) const)
	{
		Tret(*of_new_type)(Tparams...) = nullptr;
		const std::string declaration = util::AS_create_function_declaration(pName, of_new_type) + " const";

		const int r = mEngine->RegisterObjectMethod(pObject.c_str(), declaration.c_str()
			, AS::asSMethodPtr<sizeof(void (Tclass::*)())>::Convert(mFunction)
			, AS::asCALL_THISCALL);
		assert(r >= 0);
	}

	template<typename Tclass, typename Tmember>
	void add_member(const std::string& pObject, const std::string& pName, Tmember Tclass:: *pMember)
	{
		const size_t member_offset = util::data_member_offset(pMember);
		const std::string declaration = util::AS_type_to_string<Tmember>().string() + " " + pName;
		const int r = mEngine->RegisterObjectProperty(pObject.c_str(), declaration.c_str(), member_offset);
		assert(r >= 0);
	}

	template<typename Tclass, typename...Tparams>
	void add_constructor(const std::string& pObject)
	{
		void(*of_new_type)(Tparams...) = nullptr;
		const std::string declaration = util::AS_create_function_declaration("f", of_new_type);

		const int r = mEngine->RegisterObjectBehaviour(pObject.c_str(), AS::asBEHAVE_CONSTRUCT, declaration.c_str()
			, AS::asFunctionPtr(script_system::script_constructor<Tclass, Tparams...>)
			, AS::asCALL_CDECL_OBJFIRST);
		assert(r >= 0);
	}

	void abort_all();
	int tick();

	int get_current_line();
	std::string get_current_file() const;

	void set_namespace(const std::string& pName);
	void reset_namespace();

	bool is_executing();

	void throw_exception(const std::string& pMessage);

	template<typename T>
	AS_array<T>* create_array(size_t pSize = 0)
	{
		AS::asITypeInfo* type = mEngine->GetTypeInfoByDecl(util::AS_type_to_string<AS_array<T>>().string().c_str());
		AS::CScriptArray* arr = AS::CScriptArray::Create(type, pSize);
		assert(arr != nullptr);
		return static_cast<AS_array<T>*>(arr);
	}

private:

	struct thread
	{
		AS::asIScriptContext* context;
		bool keep_context;
	};

	std::shared_ptr<thread> create_thread(AS::asIScriptFunction *pFunc, bool keep_context = false);
	
	template<typename T>
	static void script_default_constructor(void *pMemory)
	{
		new(pMemory) T();
	}

	template<typename T, typename...Targs>
	static void script_constructor(void *pMemory, Targs...pArgs)
	{
		new(pMemory) T(std::forward<Targs>(pArgs)...);
	}

	template<typename T>
	static void script_default_deconstructor(void *pMemory)
	{
		((T*)pMemory)->~T();
	}

	// These are kept for the "special" functions
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);

	engine::timer mTimeout_timer;

	std::shared_ptr<thread> mCurrect_thread_context;
	std::vector<std::shared_ptr<thread>> mThread_contexts;

	AS::asIScriptEngine* mEngine;

	void register_vector_type();
	void register_timer_type();
	void message_callback(const AS::asSMessageInfo * msg);

	void script_abort();
	void script_debug_print(const std::string &pMessage);
	void script_error_print(const std::string &pMessage);
	void script_create_thread(AS::asIScriptFunction *func, AS::CScriptDictionary *arg);
	void script_create_thread_noargs(AS::asIScriptFunction *func);
	bool script_yield();

	std::map<std::string, AS::CScriptHandle> mShared_handles;

	void script_make_shared(AS::CScriptHandle pHandle, const std::string& pName);
	AS::CScriptHandle script_get_shared(const std::string& pName);

	void load_script_interface();

	void timeout_callback(AS::asIScriptContext *ctx);

	friend class script_function;
	friend class scene_script_context;
};

}

namespace logger
{
	static inline message print(rpg::script_system& pScript_system
		, level pType, const std::string& pMessage)
	{
		return print(pScript_system.get_current_file()
			, pScript_system.get_current_line()
			, pType, pMessage);
	}
}


#endif // !RPG_SCRIPT_SYSTEM_HPP
