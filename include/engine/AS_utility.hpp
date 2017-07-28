#ifndef AS_UTILITY_HPP
#define AS_UTILITY_HPP

#include <functional>
#include <type_traits>
#include <string>
#include <array>


namespace util {

class AS_type_to_string_base
{
public:
	AS_type_to_string_base() {};

	std::string string(bool mIs_param = false)
	{
		std::string ret;
		if (!mPrefix.empty()) ret  = mPrefix + " ";
		if (!mName.empty())   ret += mName;
		if (!mSuffix.empty()) ret += " " + mSuffix;
		if (!mEx_suffix.empty() && mIs_param) ret += " " + mEx_suffix;
		return ret;
	}

protected:
	std::string mPrefix;
	std::string mName;
	std::string mSuffix;
	std::string mEx_suffix;
};

template<typename T>
struct AS_type_to_string :
	AS_type_to_string_base
{
	static_assert("Unkown type");
	AS_type_to_string() {}
};

template<>
struct AS_type_to_string<void> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "void";
	}
};

template<>
struct AS_type_to_string<int> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "int";
	}
};

template<>
struct AS_type_to_string<float> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "float";
	}
};

template<>
struct AS_type_to_string<bool> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "bool";
	}
};

template<>
struct AS_type_to_string<unsigned int> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "uint";
	}
};


template<>
struct AS_type_to_string<std::string> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "string";
	}
};

template<typename T>
struct AS_type_to_string <const T> :
	AS_type_to_string<T>
{
	AS_type_to_string()
	{
		mPrefix = "const";
	}
};

template<typename T>
struct AS_type_to_string <T*> :
	AS_type_to_string<T>
{
	AS_type_to_string()
	{
		mSuffix = "@";
	}
};

template<typename T>
struct AS_type_to_string <T&> :
	AS_type_to_string<T>
{
	AS_type_to_string()
	{
		mSuffix = "&";
		mEx_suffix = "in";
	}
};

template<typename T>
struct AS_type_to_string <const T&> :
	AS_type_to_string<T>
{
	AS_type_to_string()
	{
		mPrefix = "const";
		mSuffix = "&";
		mEx_suffix = "in";
	}
};

namespace priv
{

template<typename T1>
inline void AS_create_function_declaration_args2(std::string* pVars)
{
	pVars[0] = AS_type_to_string<T1>().string(true);
}

template<typename T1, typename T2>
inline void AS_create_function_declaration_args2(std::string* pVars)
{
	pVars[0] = AS_type_to_string<T1>().string(true);
	pVars[1] = AS_type_to_string<T2>().string(true);
}
template<typename T1, typename T2, typename T3>
inline void AS_create_function_declaration_args2(std::string* pVars)
{
	pVars[0] = AS_type_to_string<T1>().string(true);
	pVars[1] = AS_type_to_string<T2>().string(true);
	pVars[2] = AS_type_to_string<T3>().string(true);
}

template<typename T1, typename T2, typename T3, typename T4>
inline void AS_create_function_declaration_args2(std::string* pVars)
{
	pVars[0] = AS_type_to_string<T1>().string(true);
	pVars[1] = AS_type_to_string<T2>().string(true);
	pVars[2] = AS_type_to_string<T3>().string(true);
	pVars[3] = AS_type_to_string<T4>().string(true);
}

template<typename T1, typename T2, typename T3, typename T4, typename T5>
inline void AS_create_function_declaration_args2(std::string* pVars)
{
	pVars[0] = AS_type_to_string<T1>().string(true);
	pVars[1] = AS_type_to_string<T2>().string(true);
	pVars[2] = AS_type_to_string<T3>().string(true);
	pVars[3] = AS_type_to_string<T4>().string(true);
	pVars[4] = AS_type_to_string<T5>().string(true);
}

template<typename...Tparams>
inline std::string AS_create_function_declaration_args()
{
	std::array<std::string, sizeof...(Tparams)> vars;
	AS_create_function_declaration_args2<Tparams...>(&vars[0]);
	std::string ret;
	for (size_t i = 0; i < vars.size(); i++)
	{
		ret += vars[i];
		if (i != vars.size() - 1)
			ret += ", ";
	}
	return ret;
}

}

// A pretty janky angelscript function declaration generator
template<typename Tret, typename...Tparams>
inline std::string AS_create_function_declaration(const std::string& pName, Tret(*)(Tparams...) = nullptr)
{
	const std::string def_args
		= priv::AS_create_function_declaration_args<Tparams...>();

	const std::string def_ret = AS_type_to_string<Tret>().string();
	return def_ret + " " + pName + "(" + def_args + ")";
}

// A pretty janky angelscript function declaration generator
template<typename Tret>
inline std::string AS_create_function_declaration(const std::string& pName, Tret(*)() = nullptr)
{
	const std::string def_ret = AS_type_to_string<Tret>().string();
	return def_ret + " " + pName + "()";
}


}

#endif