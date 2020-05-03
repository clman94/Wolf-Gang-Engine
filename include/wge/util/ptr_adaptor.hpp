#pragma once

namespace wge::util
{

// Converts a value, raw pointer, or reference into an object
// accessable only through the -> or * operators.
// References lvalues and copies rvalues and pointers.
template <typename T>
class ptr_adaptor
{
public:
	constexpr ptr_adaptor(T&& pValue) :
		value(pValue){}

	constexpr T& operator*() noexcept
	{
		return value;
	}

	constexpr const T& operator*() const noexcept
	{
		return value;
	}

	constexpr T* operator->() noexcept
    {
        return &value;
    }

	constexpr const T* operator->() const noexcept
    {
        return &value;
    }

private:
    T value;
};

template <typename T>
class ptr_adaptor<T*>
{
public:
	constexpr ptr_adaptor(T* pValue) noexcept :
		value(pValue) {}

	constexpr T& operator*() const noexcept
	{
		return *value;
	}

	constexpr T* operator->() const noexcept
    {
        return value;
    }
    
private:
    T* value;
};

template <typename T>
class ptr_adaptor<T&>
{
public:
	constexpr ptr_adaptor(T& pValue) noexcept :
		value(&pValue){}

	constexpr T& operator*() const noexcept
	{
		return *value;
	}

	constexpr T* operator->() const noexcept
    {
        return value;
    }
    
private:
    T* value;
};

template <typename T>
ptr_adaptor(T&&) -> ptr_adaptor<T>;

} // namespace wge::util
