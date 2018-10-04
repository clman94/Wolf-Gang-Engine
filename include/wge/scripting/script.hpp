#pragma once

// Angelscript
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scripthandle/scripthandle.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>

#include <wge/logging/log.hpp>
#include <wge/core/system.hpp>

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
		using namespace AngelScript;
		asDWORD typeid_flags = 0;
		pGen->GetArgTypeId(pIndex, &typeid_flags);
		if (typeid_flags & asTM_INREF || typeid_flags & asTM_OUTREF ||
			typeid_flags & asTM_INOUTREF)
		{
			T* ptr = *static_cast<T**>(pGen->GetAddressOfArg(pIndex));
			return *ptr;
		}
		else
		{
			T* ptr = static_cast<T*>(pGen->GetAddressOfArg(pIndex));
			return *ptr;
		}
	}
};

template <typename T>
struct generic_getter<T*>
{
	static T* get(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		return &generic_getter<T>::get(pGen, pIndex);
	}
};

template <typename T>
struct generic_getter<T&>
{
	static T& get(AngelScript::asIScriptGeneric* pGen, std::size_t pIndex)
	{
		return generic_getter<T>::get(pGen, pIndex);
	}
};

template <typename...Tparams>
struct function_params
{
	constexpr static const std::size_t size = sizeof...(Tparams);
};


template <typename Tret, typename Tparams, bool Tis_member = false, bool Tis_const = false>
struct function_signature
{
	using return_type = Tret;
	using param_types = Tparams;
	constexpr static const bool is_member = Tis_member;
	constexpr static const bool is_const = Tis_const;
};

template <typename T, typename...Trest>
struct first_param
{
	using type = T;
	using rest = function_params<Trest...>;
};

template <typename T, typename...Trest>
struct first_param<function_params<T, Trest...>>
{
	using type = T;
	using rest = function_params<Trest...>;
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
	using class_type = Tclass;
	using type = function_signature<Tret, function_params<Tparams...>, true>;
};


template <typename Tret, typename Tclass, typename...Tparams>
struct function_signature_traits<Tret(Tclass::*)(Tparams...) const>
{
	using class_type = std::add_const_t<Tclass>;
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
struct generic_constructor_binding :
	generic_function_binding
{
	generic_constructor_binding() = default;
	generic_constructor_binding(const generic_constructor_binding&) = default;
	generic_constructor_binding(generic_constructor_binding&&) = default;
	generic_constructor_binding(const generic_function_binding& pB) :
		generic_function_binding(pB)
	{}
	generic_constructor_binding(generic_function_binding&& pB) :
		generic_function_binding(pB)
	{}
};
struct generic_destructor_binding :
	generic_function_binding
{
	generic_destructor_binding() = default;
	generic_destructor_binding(const generic_destructor_binding&) = default;
	generic_destructor_binding(generic_destructor_binding&&) = default;
	generic_destructor_binding(const generic_function_binding& pB) :
		generic_function_binding(pB)
	{}
	generic_destructor_binding(generic_function_binding&& pB) :
		generic_function_binding(pB)
	{}
};

// Handles the return value
template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
auto wrap_return(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const>)
{
	return [pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric* pGen, Tparams&&... pParams)
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
		std::invoke(wrap_return(pFunc, pSig), pGen,
			std::forward<Tparams>(generic_getter<std::remove_reference_t<Tparams>>::get(pGen, pParams_index))...);
	};
	return generic_function_binding::create(std::move(wrapper), pSig);
}

template <typename Tfunc, bool pIs_const, typename Tret, typename Tobject, typename...Tparams>
generic_function_binding make_object_proxy(Tfunc&& pFunc, function_signature<Tret, function_params<Tobject, Tparams...>, true, pIs_const>)
{
	auto object_wrapper = [pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric* pGen, Tparams&&... pParams)
	{
		assert(pGen->GetObject());
		return std::invoke(pFunc, static_cast<Tobject>(pGen->GetObject()), std::forward<Tparams>(pParams)...);
	};
	return make_generic_callable(std::move(object_wrapper),
		function_signature<Tret, function_params<Tparams...>, true, pIs_const>{},
		std::index_sequence_for<Tparams...>{});
}

// Pretty much do nothing since it is not a class object
template <typename Tfunc, typename Tret, typename...Tparams>
generic_function_binding make_object_proxy(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, false, false> pSig)
{
	auto object_wrapper = [pFunc = std::forward<Tfunc>(pFunc)](AngelScript::asIScriptGeneric*, Tparams&&... pParams)
	{
		return std::invoke(pFunc, std::forward<Tparams>(pParams)...);
	};
	return make_generic_callable(std::move(object_wrapper), pSig, std::index_sequence_for<Tparams...>{});
}

} // namespace detail

// The first parameter will recieve the "this" pointer.
// First parameter must be pointer. If the first parameter
// is const, the function will be registered as a "const" method.
struct this_first_t {};
constexpr const this_first_t this_first;

template <typename T>
detail::generic_function_binding function(this_first_t, T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using class_type = detail::first_param<traits::type::param_types>::type;

	static_assert(traits::type::param_types::size > 0, "You need to provide at least one parameter for method function");
	static_assert(std::is_pointer_v<class_type>, "First parameter of method function needs to be a pointer");

	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types, true, std::is_const_v<class_type>>;

	return detail::make_object_proxy(std::forward<T>(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
detail::generic_function_binding function(this_first_t, Tret(*pFunc)(Tparams...))
{
	static_assert(sizeof...(Tparams) > 0, "You need to provide at least one parameter for method function");
	using class_type = detail::first_param<Tparams...>::type;

	static_assert(std::is_pointer_v<class_type>, "First parameter of method function needs to be a pointer");

	using sig = detail::function_signature<Tret, detail::function_params<Tparams...>, true, std::is_const_v<class_type>>{};

	return detail::make_object_proxy(std::move(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
detail::generic_function_binding function(Tret(*pFunc)(Tparams...))
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tparams...>>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...))
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tclass*, Tparams...>, true>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...) const)
{
	return detail::make_object_proxy(std::move(pFunc), detail::function_signature<Tret, detail::function_params<const Tclass*, Tparams...>, true, true>{});
}

template <typename T>
detail::generic_function_binding function(T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types>;
	return detail::make_object_proxy(std::forward<T>(pFunc), sig{});
}

// Bind a member function
template <typename T, typename Tinstance>
detail::generic_function_binding function(T&& pFunc, Tinstance&& pInstance)
{
	return detail::make_object_proxy(bind_mem_fn(pFunc, std::forward<Tinstance>(pInstance)));
}

template <typename T, typename...Tparams>
detail::generic_constructor_binding constructor()
{
	auto c = [](T* pLocation, Tparams&&...pArgs)
	{
		new(pLocation) T(std::forward<Tparams>(pArgs)...);
	};
	detail::generic_constructor_binding binding = scripting::function(this_first, c);
	return binding;
}

template <typename T>
detail::generic_destructor_binding destructor()
{
	auto d = [](T* pLocation)
	{
		pLocation->~T();
	};
	detail::generic_destructor_binding binding = scripting::function(this_first, d);
	return binding;
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
	detail::member_binding binding;
	// Expanded asOFFSET here and added some parenthesis because
	// the reinterpret_cast is being converted to Tclass** for some
	// odd reason. Is this perhaps a Visual Studio thing?
	binding.offset = ((std::size_t)(&(reinterpret_cast<Tclass*>(100000)->*pMember)) - 100000);
	binding.type = detail::type_info::create<Ttype>();
	return binding;
}

class script;

class script_object
{
public:

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
class script :
	public core::system
{
	WGE_SYSTEM("Script", 8042);
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
	virtual ~script()
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
		auto mod = mEngine->GetModule(mCurrent_module.c_str());
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
		int r = mEngine->RegisterObjectType(pIdentifier.c_str(), sizeof(T),
			AngelScript::asOBJ_VALUE | AngelScript::asGetTypeTraits<T>());
		WGE_ASSERT(r >= 0);
	}

	// Register a reference type.
	template <typename T>
	void reference(const std::string& pName,
		const detail::generic_function_binding& pFactory,
		const detail::generic_function_binding& pAddref,
		const detail::generic_function_binding& pRelref)
	{
		type<T>(pName);
		int r = mEngine->RegisterObjectType(pName.c_str(), 0, AngelScript::asOBJ_REF);
		WGE_ASSERT(r >= 0);
		register_object_behavior(pObject, pFactory, AngelScript::asBEHAVE_FACTORY);
		register_object_behavior(pObject, pAddref, AngelScript::asBEHAVE_ADDREF);
		register_object_behavior(pObject, pRelref, AngelScript::asBEHAVE_RELEASE);
	}

	// Register a reference type with no reference counting.
	template <typename T>
	void reference(const std::string& pName)
	{
		type<T>(pName);
		int r = mEngine->RegisterObjectType(pName.c_str(), 0, AngelScript::asOBJ_REF | AngelScript::asOBJ_NOCOUNT);
		WGE_ASSERT(r >= 0);
	}
	
	void object(const std::string& pObject, const detail::generic_constructor_binding& pConstructor)
	{
		register_object_behavior(pObject, pConstructor, AngelScript::asBEHAVE_CONSTRUCT);
	}

	void object(const std::string& pObject, const detail::generic_destructor_binding& pDestructor)
	{
		register_object_behavior(pObject, pDestructor, AngelScript::asBEHAVE_DESTRUCT);
	}

	void object(const std::string& pIdentifier, const std::string& pMember_name, const detail::member_binding& pMember)
	{
		int r = mEngine->RegisterObjectProperty(pIdentifier.c_str(),
			get_variable_declaration(pMember_name, pMember.type).c_str(), pMember.offset);
		WGE_ASSERT(r >= 0);
	}

	void object(const std::string& pIdentifier, const std::string& pFunction_name, const detail::generic_function_binding& pFunction)
	{
		using namespace AngelScript;

		const std::string declaration = get_function_declaration(pFunction_name, pFunction.types, pFunction.is_const_member);

		// Store the function object so it stays alive during the scripts lifetime
		detail::generic_function& func = mFunction_cache.emplace_back(pFunction.function);

		// Register with the generic_function_caller function. This function will delegate the call to the function object above
		// through the generic's auxilary.
		int r = mEngine->RegisterObjectMethod(pIdentifier.c_str(), declaration.c_str(),
			asFUNCTION(&script::generic_function_caller), asCALL_GENERIC, &func);
		WGE_ASSERT(r >= 0);
	}

	// Register a global property
	template <typename T>
	void global(const std::string& pIdentifier, const std::reference_wrapper<T>& pReference)
	{
		const std::string declaration = get_variable_declaration(pIdentifier, detail::type_info::create<T>());
		int r = mEngine->RegisterGlobalProperty(declaration.c_str(),
			const_cast<std::remove_const_t<T>*>(&pReference.get()));
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

	template <typename T>
	void enumerator(const std::string& pName)
	{
		static_assert(std::is_enum_v<T>, "This needs to be an enum type");
		type<T>(pName);
		int r = mEngine->RegisterEnum(pName.c_str());
		WGE_ASSERT(r >= 0);
	}

	template <typename T>
	void enumerator(const std::string& pName, T pValue)
	{
		static_assert(std::is_enum_v<T>, "This needs to be an enum type");
		WGE_ASSERT(mTypenames.find(typeid(T)) != mTypenames.end());
		int r = mEngine->RegisterEnumValue(mTypenames[typeid(T)].c_str(), pName.c_str(), static_cast<int>(pValue));
		WGE_ASSERT(r >= 0);
	}

	// Registers a type identifier.
	// It is required to register your type before
	// registering anything else that uses it.
	template <typename T>
	void type(const std::string& pIdentifier)
	{
		WGE_ASSERT(mTypenames.find(typeid(T)) == mTypenames.end());
		std::string ns = mEngine->GetDefaultNamespace();
		if (!ns.empty()) ns += "::";
		mTypenames[typeid(T)] = ns + pIdentifier;
	}

	void remove_module(const std::string& pName)
	{
		mEngine->DiscardModule(pName.c_str());
		if (mCurrent_module == pName)
			mCurrent_module.clear();
	}

	void module(const std::string& pName)
	{
		mCurrent_module = pName;
	}

	void new_module(const std::string& pName)
	{
		remove_module(pName);
		mCurrent_module = pName;
		WGE_SASSERT(mBuilder.StartNewModule(mEngine, pName.c_str()) >= 0);
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
		WGE_ASSERT(!mCurrent_module.empty());
		mBuilder.AddSectionFromFile(pFilename);
	}

	void memory(const char* pName, const char* pScript)
	{
		WGE_ASSERT(!mCurrent_module.empty());
		WGE_SASSERT(mBuilder.AddSectionFromMemory(pName, pScript));
	}

	void build()
	{
		WGE_ASSERT(!mCurrent_module.empty());
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
		using namespace AngelScript;
		// A fold expression will help set all the arguments
		([&]()
		{
			asDWORD typeid_flags = 0;
			int arg_typeid = 0;
			pCtx->GetFunction()->GetParam(I, &arg_typeid, &typeid_flags);
			if (std::is_reference_v<Tparams>)
			{
				using type = std::remove_reference_t<Tparams>;
				type** ptr = static_cast<type**>(pCtx->GetAddressOfArg(I));
				*ptr = &pParams;
			}
			else if (arg_typeid & asTYPEID_APPOBJECT)
			{
				using type = std::decay_t<Tparams>;
				type** ptr = static_cast<type**>(pCtx->GetAddressOfArg(I));
				*ptr = new type{ std::forward<Tparams>(pParams) };
			}
			else
			{
				using type = std::decay_t<Tparams>;
				type* ptr = static_cast<type*>(pCtx->GetAddressOfArg(I));
				*ptr = pParams;
			}
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
			WGE_ASSERT(ctx->Prepare(pFunc) >= 0);
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
		WGE_ASSERT(mTypenames.find(*pType.stdtypeinfo) != mTypenames.end());
		std::string result;
		if (pType.is_const)
			result += "const ";
		result += mTypenames.find(*pType.stdtypeinfo)->second + " " + pIdentifier;
		return result;
	}

	// Generates a string used as a return for a function declaration
	std::string get_return_declaration(const detail::type_info& pType) const
	{
		WGE_ASSERT(mTypenames.find(*pType.stdtypeinfo) != mTypenames.end());
		std::string result;
		if (pType.is_const)
			result += "const ";
		result += mTypenames.find(*pType.stdtypeinfo)->second;
		if (pType.is_pointer)
			result += "&";
		return result;
	}

	// Generates a parameter
	// [const] TYPE [&in]
	std::string get_parameter_declaration(const detail::type_info& pType) const
	{
		WGE_ASSERT(mTypenames.find(*pType.stdtypeinfo) != mTypenames.end());
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
		std::string result;
		result += get_return_declaration(pTypes.return_type);
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

	void register_object_behavior(const std::string& pObject,
		const detail::generic_function_binding& pFunction,
		AngelScript::asEBehaviours pBehavior)
	{
		using namespace AngelScript;

		const std::string declaration = get_function_declaration("f", pFunction.types, pFunction.is_const_member);

		// Preserve the lifetime fo the function
		detail::generic_function& func = mFunction_cache.emplace_back(pFunction.function);

		// Register the behavior
		int r = mEngine->RegisterObjectBehaviour(pObject.c_str(), pBehavior, declaration.c_str(),
			asFUNCTION(&script::generic_function_caller), AngelScript::asCALL_GENERIC, &func);
		WGE_ASSERT(r >= 0);
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

	std::string mCurrent_module;
};

} // namespace wge::scripting
