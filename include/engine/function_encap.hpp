#ifndef UTIL_FUNCTION_ENCAP_HPP
#define UTIL_FUNCTION_ENCAP_HPP

#include <functional>

namespace util {

namespace priv
{
// Helps order the arguments correctly
template<std::size_t ...I>
struct indices {};

template<std::size_t N, std::size_t ...I>
struct index_factory : index_factory<N - 1, N - 1, I...>
{};

template<std::size_t ...I>
struct index_factory<0, I...>
{
	typedef indices<I...> type;
};

template<typename Tret, typename...Tparams, std::size_t ...I>
inline Tret call_encapsulated_args(Tret(*pFunc)(Tparams...), void** pArgs, indices<I...>)
{
	try {
		return pFunc(*reinterpret_cast<typename std::remove_reference<Tparams>::type*>(pArgs[I])...);
	}
	catch (...)
	{
		std::cout << "error";
		return Tret();
	}
}

template<typename T>
void increment_pack_counter(size_t& i)
{
	++i;
}
}

template<typename Tret, typename...Tparams>
inline size_t get_function_param_count(Tret(*pFunc)(Tparams...))
{
	size_t i = 0;
	(priv::increment_pack_counter<Tparams>(i), ...);
	return i;
}

// Calls a function with manually specified arguments.
// Allows the creation of wrappers where parameters are not explicitly known.
template<typename Tret, typename...Tparams>
inline Tret call_encapsulated_args(Tret(*pFunc)(Tparams...), void** pArgs)
{
	return priv::call_encapsulated_args(pFunc, pArgs, typename priv::index_factory<sizeof...(Tparams)>::type());
}

// No parameters to expand.
template<typename Tret>
inline Tret call_encapsulated_args(Tret(*pFunc)())
{
	return pFunc();
}

typedef std::function<void(void*, void**)> encapsulated_function;

template<typename...Tparams>
inline encapsulated_function encapsulate_function(void(*pFunc)(Tparams...))
{
	return [=](void* ret, void** args)->void*
	{
		call_encapsulated_args(pFunc, args);
	};
}

template<typename Tret, typename...Tparams>
inline encapsulated_function encapsulate_function(Tret(*pFunc)(Tparams...))
{
	return [=](void* ret, void** args)->void*
	{
		if (ret == nullptr)
			call_implicit_args(pFunc, args);
		else
			*reinterpret_cast<Tret*>(ret) = call_encapsulated_args(pFunc, args);
	};
}

}

#endif