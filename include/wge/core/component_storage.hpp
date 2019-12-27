#pragma once

#include <optional>
#include <vector>

#include <wge/core/object_id.hpp>
#include <wge/util/uuid.hpp>
#include <wge/util/ipair.hpp>
#include <wge/util/ptr_adaptor.hpp>
#include <wge/util/span.hpp>

namespace wge::core
{

// O(1) worst case lookup, O(n) worst case insert, O(1) worst case removal.
template <typename T>
struct sparse_set
{
public:
	using key = std::size_t;

	class iterator
	{
		friend sparse_set<T>;
	public:
		using difference_type = std::ptrdiff_t;
		using size_type = std::size_t;
		using reference = std::pair<const key, T&>;
		using value_type = std::pair<key, T>;

	private:
		iterator(std::vector<key>& pKeys,
			std::vector<T>& pValues,
			difference_type pIndex) noexcept :
			mKeys(&pKeys),
			mValues(&pValues),
			mIndex(pIndex)
		{}

	public:
		iterator() noexcept = default;

		void next() noexcept
		{
			++mIndex;
		}

		void previous() noexcept
		{
			assert(mIndex > 0);
			--mIndex;
		}

		auto get() const noexcept
		{
			return reference{ mKeys->at(mIndex), mValues->at(mIndex) };
		}

		iterator& operator++() noexcept
		{
			next();
			return *this;
		}

		iterator operator++(int) noexcept
		{
			iterator temp = *this;
			next();
			return temp;
		}

		iterator& operator--() noexcept
		{
			previous();
			return *this;
		}

		iterator operator--(int) noexcept
		{
			iterator temp = *this;
			previous();
			return temp;
		}

		bool operator==(const iterator& pR) const noexcept
		{
			return mKeys == pR.mKeys && mIndex == pR.mIndex;
		}

		bool operator!=(const iterator& pR) const noexcept
		{
			return !operator==(pR);
		}

		auto operator->() const noexcept
		{
			return util::ptr_adaptor{ get() };
		}

		auto operator*() const noexcept
		{
			return get();
		}

	private:
		std::vector<key>* mKeys = nullptr;
		std::vector<T>* mValues = nullptr;
		difference_type mIndex = 0;
	};

	bool has(key pKey) const noexcept
	{
		if (pKey < mMinimum || mLookup.empty())
			return false;
		std::size_t offset = get_lookup_index(pKey);
		if (offset >= mLookup.size())
			return false;
		return mLookup[offset].has_value();
	}

	T* get(key pKey)
	{
		if (!has(pKey))
			return nullptr;

		auto offset = get_lookup_index(pKey);
		auto index = mLookup[offset];
		return &mValues[index.value()];
	}

	auto at(std::size_t pIndex)
	{
		return std::pair<object_id, T&>(mKeys[pIndex], mValues[pIndex]);
	}

	template <typename Tvalue = T>
	T& insert(key pKey, Tvalue&& pValue = T{})
	{
		// Expand the lookup to the left to accommodate the new key.
		if (pKey < mMinimum)
		{
			auto difference = mMinimum - pKey;
			mLookup.resize(mLookup.size() + difference);
			std::move_backward(mLookup.begin(),
				mLookup.end() - difference, mLookup.end());
			mMinimum = pKey;
		}
		// Set the minimum to the first key.
		else if (mLookup.empty())
			mMinimum = pKey;
		std::size_t offset = get_lookup_index(pKey);
		// Expand the lookup to the right if needed.
		if (offset >= mLookup.size())
			mLookup.resize(offset + 1);

		// Already exists?
		else if (mLookup[offset].has_value())
			return mValues[mLookup[offset].value()];

		// Insert the new information.
		mLookup[offset] = std::make_optional(mValues.size());
		mKeys.push_back(pKey);
		return mValues.emplace_back(std::forward<Tvalue>(pValue));
	}

	T& operator[](key pKey)
	{
		if (auto* v = get(pKey))
			return *v;
		else
			return insert(pKey, 0);
	}

	void remove(key pKey)
	{
		if (!has(pKey))
			return;
		std::size_t offset = get_lookup_index(pKey);
		std::size_t index = mLookup[offset].value();
		if (index < size() - 1)
		{
			// Constant-time deletion by swapping the item with
			// the back and then popping the back.
			std::swap(mKeys[index], mKeys.back());
			std::swap(mValues[index], mValues.back());
			// Update the lookup for the item we swapped from the back.
			mLookup[get_lookup_index(mKeys[index])] = std::make_optional(index);
		}
		mKeys.pop_back();
		mValues.pop_back();
		mLookup[offset].reset();
	}

	// Resize the lookup table to conserve memory.
	void shrink_lookup()
	{
		auto has_value = [](auto& a) { return a.has_value(); };
		// Shrink from the left.
		auto first = std::find_if(mLookup.begin(), mLookup.end(), has_value);
		if (first != mLookup.end())
		{
			std::size_t amount = std::distance(mLookup.begin(), first);
			mMinimum = first->value();
			std::move(first, mLookup.end(), mLookup.begin());
			mLookup.resize(mLookup.size() - amount);
		}
		// Shrink from the right.
		auto last = std::find_if(mLookup.rbegin(), mLookup.rend(), has_value);
		if (last != mLookup.rend())
		{
			std::size_t amount = std::distance(mLookup.rbegin(), last + 1);
			mLookup.resize(mLookup.size() - amount);
		}
		mLookup.shrink_to_fit();
	}

	void clear()
	{
		mMinimum = 0;
		mLookup.clear();
		mKeys.clear();
		mValues.clear();
	}

	bool empty() const noexcept
	{
		return mKeys.empty(); // mKeys and mValues are interchangable.
	}

	std::size_t size() const noexcept
	{
		return mKeys.size(); // mKeys and mValues are interchangeable.
	}

	auto begin() noexcept
	{
		return iterator(mKeys, mValues, 0);
	}

	auto end() noexcept
	{
		return iterator(mKeys, mValues, size());
	}

	std::size_t lookup_size() const noexcept
	{
		return mLookup.size();
	}

	auto get_raw() noexcept
	{
		return util::span{ mValues };
	}

	auto get_raw() const noexcept
	{
		return util::span{ mValues };
	}

	auto get_const_raw() const noexcept
	{
		return util::span<const T>{ mValues };
	}

private:
	// Get the index of the item in the lookup table from a key.
	std::size_t get_lookup_index(key pKey) const noexcept
	{
		return pKey - mMinimum;
	}

	std::size_t mMinimum = 0;
	// Gives us O(1) lookup of values.
	// Because object id's are able to be reused,
	// they are usually reasonably contiguous. So,
	// we can opt for a simple lookup.
	std::vector<std::optional<std::size_t>> mLookup;
	std::vector<key> mKeys;
	std::vector<T> mValues;
};

class component_storage_base
{
public:
	virtual ~component_storage_base() {}
	virtual std::size_t size() const noexcept = 0;
	virtual void remove(const object_id& pObject_id) = 0;
};

template <typename T>
class component_storage :
	public component_storage_base,
	public sparse_set<T>
{
public:
	using sparse_set<T>::operator[];
	
	virtual std::size_t size() const noexcept
	{
		return sparse_set<T>::size();
	}

	virtual void remove(const object_id& pObject_id)
	{
		sparse_set<T>::remove(pObject_id);
	}
};

} // namespace wge::core
