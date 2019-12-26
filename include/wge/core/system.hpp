#pragma once

#include <wge/core/serialize_type.hpp>
#include <wge/util/json_helpers.hpp>

#include <string>
#include <vector>
#include <utility>
#include <any>

namespace wge::core
{

class any_set
{
public:
	template <typename T, typename...Targs>
	T* insert(Targs&&...pArgs)
	{
		if (auto existing = get<T>())
		{
			*existing = T{ std::forward<Targs>(pArgs)... };
			return existing;
		}
		else
			return &mAnys.emplace_back(std::make_any(T{ pArgs... }));
	}

	template <typename T>
	T* get() noexcept
	{
		for (auto& i : mAnys)
			if (i.type() == typeid(T))
				return std::any_cast<T>(&i);
		return nullptr;
	}

	template <typename T>
	const T* get() const noexcept
	{
		for (auto& i : mAnys)
			if (i.type() == typeid(T))
				return std::any_cast<T>(&i);
		return nullptr;
	}

	template <typename T>
	bool has() const noexcept
	{
		return get<T>() != nullptr;
	}

	bool empty() const noexcept
	{
		return mAnys.empty();
	}

	std::size_t size() const noexcept
	{
		return mAnys.size();
	}

private:
	std::vector<std::any> mAnys;
};

} // namespace wge::core
