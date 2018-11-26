#pragma once

#include <cstdint>

namespace wge::core
{

using instance_id_t = std::uint64_t;

class instance_id
{
public:
	instance_id() = default;
	instance_id(instance_id_t pVal) :
		mValue(pVal)
	{
	}

	bool operator<(const instance_id& pR) const
	{
		return mValue < pR.mValue;
	}

	bool operator==(const instance_id& pR) const
	{
		return mValue == pR.mValue;
	}
	
	instance_id_t get_value() const
	{
		return mValue;
	}

	void set_value(instance_id_t pId)
	{
		mValue = pId;
	}

private:
	instance_id_t mValue{ 0 };
};

} // namespace wge::core
