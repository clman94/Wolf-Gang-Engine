#ifndef DICTIONARY_HPP
#define DICTIONARY_HPP

#include <vector>
#include <list>

namespace engine{

template<typename T>
struct dictionary_entry
{
	std::string key;
	T item;
};

template<typename T>
struct dictionary_list
	: public std::list<dictionary_entry<T>>
{
	T* find_item(const std::string key)
	{
		for (auto& i : *this)
		{
			if (i.key == key)
				return &i.item;
		}
		return nullptr;
	}
};

}

#endif