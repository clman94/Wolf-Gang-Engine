
// Angelscript
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scripthandle/scripthandle.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <wge/logging/log.hpp>

#include <functional>
#include <cassert>
#include <typeindex>
#include <map>
#include <vector>
#include <type_traits>
#include <string>
#include <cstddef>
#include <cstdint>

namespace wge::scripting
{


namespace detail
{

using generic_function = std::function<void(AngelScript::asIScriptGeneric*)>;

template <typename Tret, typename Tclass, typename...Tparams, typename Tinstance>
auto bind_mem_fn(Tret(Tclass::* pFunc)(Tparams...), Tinstance&& pInstance)
{
	return[pFunc, pInstance = std::forward<Tinstance>(pInstance)](Tparams&&... pParams)
	{
		return std::invoke(pFunc, pInstance, std::forward<Tparams>(pParams)...);
	};
}

template <typename T>
struct generic_getter
{
	static T& get(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		// For some odd reason, angelscript only dereferences object values
		// and nothing else.
		if (auto obj = pGen->GetArgObject(pIndex))
		{
			return *static_cast<T*>(obj);
		}
		else
		{
			auto ptr = *static_cast<T**>(pGen->GetAddressOfArg(pIndex));
			return *ptr;
		}
	}
};

template <typename T>
struct generic_getter<T*>
{
	static T* get(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		if (auto obj = pGen->GetArgObject(pIndex))
		{
			return static_cast<T*>(obj);
		}
		else
		{
			return *static_cast<T**>(pGen->GetAddressOfArg(pIndex));
		}
	}
};

template <typename T>
struct generic_getter<T&>
{
	static T& get(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		return *(*static_cast<T**>(pGen->GetAddressOfArg(pIndex)));
	}
};

template <typename T>
struct generic_returner
{
	static auto set(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		return std::forward<T*>(static_cast<T*>(pGen->GetAddressOfArg(pIndex)));
	}
};

template <typename T>
struct generic_returner<T*>
{
	static auto set(AngelScript::asIScriptGeneric* pGen, T* pVal)
	{
		pGen->SetReturnAddress(pVal);
		return std::forward<T*>(static_cast<T*>(pGen->GetAddressOfArg(pIndex)));
	}
};

template <typename...Tparams>
struct function_params {};

template <typename Tret, typename Tparams, bool Tis_member = false, bool Tis_const = false>
struct function_signature
{
	using return_type = Tret;
	using param_types = Tparams;
	constexpr static const bool is_member = Tis_member;
	constexpr static const bool is_const = Tis_const;
};

template <typename T>
struct function_signature_traits {};

template <typename Tret, typename...Tparams>
struct function_signature_traits<Tret(*)(Tparams...)>
{
	using type = function_signature<Tret, function_params<Tparams...>>;
};

template <typename Tret, typename Tclass, typename...Tparams>
struct function_signature_traits<Tret(Tclass::*)(Tparams...)>
{
	using type_class = Tclass;
	using type = function_signature<Tret, function_params<Tparams...>, true>;
};


template <typename Tret, typename Tclass, typename...Tparams>
struct function_signature_traits<Tret(Tclass::*)(Tparams...) const>
{
	using type_class = Tclass;
	using type = function_signature<Tret, function_params<Tparams...>, true, true>;
};

struct type_info
{
	bool is_const;
	bool is_reference;
	bool is_pointer;
	const std::type_info* stdtypeinfo;

	type_info() = default;
	type_info(bool pIs_const,
		bool pIs_reference,
		bool pIs_pointer,
		const std::type_info* pType) :
		is_const(pIs_const),
		is_reference(pIs_reference),
		is_pointer(pIs_pointer),
		stdtypeinfo(pType)
	{}
	type_info(const type_info&) = default;
	type_info(type_info&&) = default;
	type_info& operator=(const type_info&) = default;

	template <typename T>
	static type_info create()
	{
		return type_info(
			std::is_const<T>::value,
			std::is_reference<T>::value,
			std::is_pointer<T>::value,
			&typeid(std::decay_t<T>)
		);
	}
};

struct function_signature_types
{
	type_info return_type;
	std::vector<type_info> param_types;

	template <typename Tret, typename...Tparams>
	static function_signature_types create()
	{
		function_signature_types types;
		types.return_type = type_info::create<Tret>();
		types.param_types = { type_info::create<Tparams>()... };
		return types;
	}

	template <bool Tis_member, bool Tis_const, typename Tret, typename...Tparams>
	static function_signature_types create(function_signature<Tret, function_params<Tparams...>, Tis_member, Tis_const>)
	{
		return create<Tret, Tparams...>();
	}
};

struct generic_function_binding
{
	function_signature_types types;
	generic_function function;
	bool is_const_member;

	template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
	static generic_function_binding create(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const>)
	{
		generic_function_binding binding;
		binding.function = pFunc;
		binding.types = function_signature_types::create<Tret, Tparams...>();
		binding.is_const_member = pIs_const;
		return binding;
	}
};

// Handles the return value
template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
auto wrap_return(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const>)
{
	return[pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric* pGen, Tparams&&... pParams)
	{
		if constexpr (std::is_same<void, Tret>::value)
		{
			// No return value
			std::invoke(pFunc, pGen, std::forward<Tparams>(pParams)...);
		}
		else
		{
			new (pGen->GetAddressOfReturnLocation()) Tret{ std::invoke(pFunc, pGen, std::forward<Tparams>(pParams)...) };
		}
	};
}

template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams, std::size_t...pParams_index>
generic_function_binding make_generic_callable(Tfunc&& pFunc,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig,
	std::index_sequence<pParams_index...>)
{
	auto wrapper = [pFunc = std::forward<Tfunc>(pFunc), pSig](AngelScript::asIScriptGeneric* pGen)
	{
		std::invoke(wrap_return(pFunc, pSig), pGen, std::forward<Tparams>(generic_getter<Tparams>::get(pGen, pParams_index))...);
	};
	return generic_function_binding::create(std::move(wrapper), pSig);
}

// If this is a class method, we need to use the GetObject() method to get the instance
template <typename Tfunc, bool pIs_const, typename Tret, typename Tobject, typename...Tparams>
generic_function_binding make_object_proxy(Tfunc&& pFunc, function_signature<Tret, function_params<Tobject, Tparams...>, true, pIs_const>)
{
	auto object_wrapper = [pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric* pGen, Tparams&&... pParams) ->auto
	{
		assert(pGen->GetObject());
		return std::invoke(pFunc, std::forward<Tobject>(*static_cast<Tobject*>(pGen->GetObject())), std::forward<Tparams>(pParams)...);
	};
	return make_generic_callable(std::move(object_wrapper), function_signature<Tret, function_params<Tparams...>, true, pIs_const>{}, std::index_sequence_for<Tparams...>{});
}

// Pretty much do nothing since it is not a class object
template <typename Tfunc, typename Tret, typename...Tparams>
generic_function_binding make_object_proxy(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, false, false> pSig)
{
	// We still have to create a wrapper function that takes a generic parameter for the next step
	auto object_wrapper = [pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric*, Tparams&&... pParams)
	{
		return std::invoke(pFunc, std::forward<Tparams>(pParams)...);
	};
	return make_generic_callable(std::move(object_wrapper), pSig, std::index_sequence_for<Tparams...>{});
}

} // namespace detail

template <typename T>
detail::generic_function_binding function(T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types>;
	return detail::make_object_proxy(std::forward<T>(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
detail::generic_function_binding function(Tret(*pFunc)(Tparams...))
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tparams...>>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...))
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tclass, Tparams...>, true>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...) const)
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tclass, Tparams...>, true, true>{});
}
// Bind a member function
template <typename T, typename Tinstance>
detail::generic_function_binding function(T&& pFunc, Tinstance&& pInstance)
{
	return detail::make_object_proxy(bind_mem_fn(pFunc, std::forward<Tinstance>(pInstance)));
}

namespace detail
{

struct member_binding
{
	std::size_t offset;
	type_info type;
};

} // namespace detail

template <typename Tclass, typename Ttype>
detail::member_binding member(Ttype Tclass::* pMember)
{
	member_binding mem;
	// Expanded asOFFSET here and added some parenthesis because
	// the reinterpret_cast is being converted to Tclass** for some
	// odd reason. Is this perhaps a Visual Studio thing?
	mem.offset = ((std::size_t)(&(reinterpret_cast<Tclass*>(100000)->*pMember)) - 100000);
	mem.type = type_info::create<Ttype>();
	return mem;
}



class script;

class script_object
{
public:
	template <typename Treturn, typename Tparams>
	void call(const std::string_view& pName, Treturn& pReturn, Tparams&& pParams...)
	{

	}

	template <typename T>
	T& get(const std::string_view& pIdentifier)
	{

	}

private:
	script* mScript;
	AngelScript::asIScriptObject* mObject;
};

// A simple callable object that, when called,
// will create a new context and execute the requested
// anglescript function.
template <typename T>
using script_function = std::function<T>;

// Wrapper for adapting the angelscript interface to modern C++.
// The goals of this project are as follows:
//   - Abstract all function and class and variable declarations.
//     What this means is that you will never retrieve a function by its raw
//     string declaration eg "void myglobalfunc(int)." Instead, you can use
//     the provided get_function<>() method to get a
//     global function eg myscript.get_function<void(int)>("myglobalfunc").
//   - Simple, type-safe registeration of variables and functions.
//     With the power of template metaprogramming, we can transform this:
//     engine->RegisterGlobalFunction("void myglobalfunc(int)", AngelScript::asFUNCTION(&myglobalfunc), AngelScript::AS);
//     ...into this:
//     myscript.global("myglobalfunc", scripting::function(&myglobalfunc));
//     As you might observe, there is no declaration, just the function identifier. With a simple
//     lookup, C++ types can be converted to angelscript types easily.
//     To register a type simply call the type<>() method.
//   - Register callable objects as functions.
//     This will allow the use of lambdas and anything else with
//     the operator() method overloaded to be registered
//     as a function.
class script
{
public:
	// Return true on success or false if your callback has failed to retrieve the include
	using include_callback = std::function<bool(const char* pInclude, const char* pFrom, script& pScript)>;

public:
	script()
	{
		using namespace AngelScript;

		mEngine = asCreateScriptEngine();

		// Register a message callback so we can see whats happening.
		// This is just copied from the angelscript documentation.
		void(*MessageCallback)(const asSMessageInfo *msg, void *param) =
			[](const asSMessageInfo *msg, void *param)
		{
			log::level level = log::level::error;
			if (msg->type == asMSGTYPE_WARNING)
				level = log::level::warning;
			else if (msg->type == asMSGTYPE_INFORMATION)
				level = log::level::info;
			log::out << level << log::line_info{ msg->row, msg->col, msg->section } << msg->message << log::endm;
		};
		mEngine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

		// Register some basic utilities
		RegisterScriptHandle(mEngine);
		RegisterScriptArray(mEngine, true);
		RegisterStdString(mEngine);
		RegisterStdStringUtils(mEngine);

		// Register the pod types
		type<void>("void");
		type<bool>("bool");

		type<std::uint8_t>("uint8");
		type<std::uint16_t>("uint16");
		type<std::uint32_t>("uint");
		type<std::uint64_t>("uint64");

		type<std::int8_t>("int8");
		type<std::int16_t>("int16");
		type<std::int32_t>("int");
		type<std::int64_t>("int64");

		type<float>("float");
		type<double>("double");

		// String type
		type<std::string>("string");
	}
	script(const script&) = delete;
	script(script&&) = delete; // Disabled for now
	~script()
	{
		mEngine->ShutDownAndRelease();
	}

	script& operator=(const script&) = delete;

	template <typename Tdeclar>
	script_function<Tdeclar> get_function(const std::string& pName)
	{
		using namespace AngelScript;
		using traits = detail::function_signature_traits<Tdeclar*>;
		const auto types = detail::function_signature_types::create(traits::type{});
		const std::string declaration = get_function_declaration(pName, types);
		//mEngine->GetGlobalFunctionByDecl
		auto mod = mEngine->GetModule("default");
		asIScriptFunction * func = nullptr;
		if (mod)
			func = mod->GetFunctionByDecl(declaration.c_str());
		else
			// Get global function from engine by default if there is no module yet
			func = mEngine->GetGlobalFunctionByDecl(declaration.c_str());
		WGE_ASSERT(func);

		return make_script_function(func, traits::type{});
	}

	// Register a value type.
	template <typename T>
	void value(const std::string& pIdentifier)
	{
		type<T>(pIdentifier);
		int r = mEngine->RegisterObjectType(pIdentifier.c_str(), sizeof(T), AngelScript::asOBJ_VALUE);
		WGE_ASSERT(r >= 0);
	}

	void object(const std::string& pIdentifier, const std::string& pMember_name, const detail::member_binding& pMember)
	{
		int r = mEngine->RegisterObjectProperty(pIdentifier.c_str(),
			get_variable_declaration(pMember_name, pMember.type).c_str(), pMember.offset);
		WGE_ASSERT(r >= 0);
	}

	void object(const std::string& pIdentifier, const std::string& pFunction_name, const detail::generic_function_binding& pFunction)
	{
		const std::string declaration = get_function_declaration(pFunction_name, pFunction.types, pFunction.is_const_member);

		// Store the function object so it stays alive during the scripts lifetime
		detail::generic_function& func = mFunction_cache.emplace_back(pFunction.function);

		// Register with the generic_function_caller function. This function will delegate the call to the function object above
		// through the generic's auxilary.
		int r = mEngine->RegisterObjectMethod(pIdentifier.c_str(), declaration.c_str(),
			AngelScript::asFUNCTION(&script::generic_function_caller), AngelScript::asCALL_GENERIC, &func);
		WGE_ASSERT(r >= 0);
	}

	// Register a global property
	template <typename T>
	void global(const std::string& pIdentifier, const std::reference_wrapper<T>& pReference)
	{
		int r = mEngine->RegisterGlobalProperty(get_variable_declaration(pIdentifier, detail::type_info::create<T>()).c_str(),
			const_cast<std::remove_const_t<T*>>(&pReference.get()));
		WGE_ASSERT(r >= 0);
	}

	// Register a global function
	void global(const std::string& pFunction_name, const detail::generic_function_binding& pFunction)
	{
		const std::string declaration = get_function_declaration(pFunction_name, pFunction.types);

		// Store the function object so it stays alive during the scripts lifetime
		detail::generic_function& func = mFunction_cache.emplace_back(pFunction.function);

		int r = mEngine->RegisterGlobalFunction(declaration.c_str(),
			AngelScript::asFUNCTION(&script::generic_function_caller), AngelScript::asCALL_GENERIC, &func);
		WGE_ASSERT(r >= 0);
	}

	void user_namespace(const char* pNamespace)
	{
		mEngine->SetDefaultNamespace(pNamespace);
	}

	void default_namespace()
	{
		mEngine->SetDefaultNamespace("");
	}

	// Registers a type identifier.
	// It is required to register your type before
	// registering anything else that uses it.
	template <typename T>
	void type(const std::string_view& pIdentifier)
	{
		WGE_ASSERT(mTypenames.find(typeid(T)) == mTypenames.end());
		mTypenames[typeid(T)] = pIdentifier;
	}

	void module()
	{
		mEngine->DiscardModule("default");
		mBuilder.StartNewModule(mEngine, "default");
	}

	// Set the include callback 
	void set_include_callback(include_callback pFunc)
	{
		using namespace AngelScript;
		mInclude_callback_info.pFunc = pFunc;
		mInclude_callback_info.pScript = this;
		INCLUDECALLBACK_t callback_delegate =
			[](const char *include, const char *from, CScriptBuilder *builder, void *userParam)->int
		{
			include_callback_info* info = static_cast<include_callback_info*>(userParam);
			return info->pFunc(include, from, *info->pScript) ? 0 : -1;
		};
		mBuilder.SetIncludeCallback(callback_delegate, &mInclude_callback_info);
	}

	void file(const char* pFilename)
	{
		mBuilder.AddSectionFromFile(pFilename);
	}

	void build()
	{
		mBuilder.BuildModule();
	}

private:
	struct include_callback_info
	{
		script* pScript;
		include_callback pFunc;
	} mInclude_callback_info;

private:

	template <typename...Tparams, std::size_t...I>
	static void set_context_args_impl(AngelScript::asIScriptContext* pCtx,
		std::index_sequence<I...>, Tparams&&... pParams)
	{
		// A fold expression will help set all the arguments
		([&]()
		{
			using type = std::decay_t<Tparams>;
			type* p_ptr = new type{ std::forward<Tparams>(pParams) };
			type** ptr = static_cast<type**>(pCtx->GetAddressOfArg(I));
			*ptr = p_ptr;
		}(), ...);
	}
	template <typename...Tparams>
	static void set_context_args(AngelScript::asIScriptContext* pCtx, Tparams&&... pParams)
	{
		set_context_args_impl(pCtx, std::index_sequence_for<Tparams...>{}, std::forward<Tparams>(pParams)...);
	}

	// Creates a callable script_function object that wraps around angelscript functions.
	template <typename Tret, typename...Tparams>
	script_function<Tret(Tparams&&...)> make_script_function(AngelScript::asIScriptFunction* pFunc,
		detail::function_signature<Tret, detail::function_params<Tparams...>>)
	{
		return [this, pFunc](Tparams&&... pArgs)->Tret
		{
			AngelScript::asIScriptContext* ctx = mEngine->RequestContext();
			assert(ctx->Prepare(pFunc) >= 0);
			set_context_args(ctx, std::forward<Tparams>(pArgs)...);
			ctx->Execute();
			ctx->Unprepare();
			mEngine->ReturnContext(ctx);
		};
	}

	// Delegates to the generic_function object pointed to in asIScriptGeneric's auxilary
	static void generic_function_caller(AngelScript::asIScriptGeneric* pGen)
	{
		detail::generic_function* func = static_cast<detail::generic_function*>(pGen->GetAuxiliary());
		(*func)(pGen);
	}

	// Generates a variable declaration
	// [const] TYPE IDENTIFIER
	std::string get_variable_declaration(const std::string& pIdentifier, const detail::type_info& pType) const
	{
		assert(mTypenames.find(*pType.stdtypeinfo) != mTypenames.end());
		std::string result;
		if (pType.is_const)
			result += "const ";
		result += mTypenames.find(*pType.stdtypeinfo)->second + " " + pIdentifier;
		return result;
	}

	// Generates a parameter
	// [const] TYPE [&in]
	std::string get_parameter_declaration(const detail::type_info& pType) const
	{
		assert(mTypenames.find(*pType.stdtypeinfo) != mTypenames.end());
		std::string result;
		if (pType.is_const)
			result += "const ";
		result += mTypenames.find(*pType.stdtypeinfo)->second;
		if (pType.is_reference)
			result += "&in";
		return result;
	}

	// Generates a function declaration
	// RETURN IDENTIFIER ( [PARAMETER [, PARAMETER]*] ) [const]
	std::string get_function_declaration(const std::string& pIdentifier,
		const detail::function_signature_types& pTypes, bool pIs_const = false) const
	{
		assert(mTypenames.find(*pTypes.return_type.stdtypeinfo) != mTypenames.end());
		std::string result;
		result += mTypenames.find(*pTypes.return_type.stdtypeinfo)->second;
		result += " ";
		result += pIdentifier;
		result += "(";
		for (auto i = pTypes.param_types.begin(); i != pTypes.param_types.end(); ++i)
		{
			result += get_parameter_declaration(*i);
			if (i + 1 != pTypes.param_types.end())
				result += ", ";
		}
		result += ")";
		if (pIs_const)
			result += " const";
		return result;
	}

private:
	// This simply stores all the function objects that have been registered.
	// It will never be accessed and only serves to keep the function objects alive.
	std::list<detail::generic_function> mFunction_cache;

	// A simple lookup to find the typename of the anglescipt
	// equivalent type.
	std::map<std::type_index, std::string> mTypenames;

	AngelScript::asIScriptEngine* mEngine;
	AngelScript::CScriptBuilder mBuilder;
};

}