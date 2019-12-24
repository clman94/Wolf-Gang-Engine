#pragma once

#include <wge/core/serialize_type.hpp>
#include <wge/util/json_helpers.hpp>

#include <string>
#include <vector>
#include <utility>
#include <any>

#define WGE_SYSTEM(name__, id__) \
	public: \
	static constexpr int SYSTEM_ID = id__; \
	static constexpr const char* SYSTEM_NAME = name__; \
	virtual int get_system_id() const override { return id__; } \
	virtual std::string get_system_name() const override { return name__; }

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

class system
{
public:
	system() {}
	virtual ~system() {}
	virtual int get_system_id() const = 0;
	virtual std::string get_system_name() const = 0;
};

} // namespace wge::core
