#pragma once

#include <string>
#include <vector>
#include <functional>

#include <wge/util/json_helpers.hpp>

namespace wge::core
{

class serializable
{
public:
	using getter_function = std::function<json()>;
	using setter_function = std::function<void(const json&)>;

	struct property
	{
		getter_function getter;
		setter_function setter;
	};

	serializable() :
		mOriginal(nullptr)
	{}

	virtual json serialize() const { return serialize_all_properties(); }
	virtual void deserialize(const json& pJson) { deserialize_all_properties(pJson); };

	json get_property(const std::string_view& pName)
	{
	}

	void set_property(const std::string_view& pName, const json& pJson)
	{

	}

protected:
	// Register a value to be referenced for serialization.
	// This assumes that the to_json and from_json overloads are available
	// for this type and that the referenced object is kept alive as long
	// as this serializable object.
	template<typename T>
	void register_property(const std::string& pName, T& pRef)
	{
		mProperties[pName] = {
			[&pRef]() -> json
			{
				return { pRef };
			},

			[&pRef](const json& pJson)
			{
				if constexpr (!std::is_const_v<T>)
					pRef = pJson;
			}
		};
	}

	void register_property(const std::string& pName, getter_function mGetter, setter_function mSetter)
	{
		mProperties[pName] = { mGetter, mSetter };
	}

	json serialize_all_properties() const
	{
		json result;
		for (auto& i : mProperties)
			result[i.first] = i.second.getter();
		return result;
	}

	void deserialize_all_properties(const json& pJson) const
	{
		for (auto& i : mProperties)
		{

		}
	}

private:
	serializable* mOriginal;
	std::map<std::string, property> mProperties;
};

}
