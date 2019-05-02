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

class layer;

class system
{
public:
	using uptr = std::unique_ptr<system>;

	system(layer& pLayer) :
		mLayer(pLayer)
	{}
	virtual ~system() {}
	virtual int get_system_id() const = 0;
	virtual std::string get_system_name() const = 0;

	json serialize(serialize_type pType = serialize_type::all)
	{
		json result;
		result["type"] = get_system_id();
		result["data"] = on_serialize(pType);
		return result;
	}
	void deserialize(const json& pJson)
	{
		on_deserialize(pJson["data"]);
	}

	layer& get_layer() const noexcept
	{
		return mLayer;
	}

	virtual void preupdate(float pDelta) {}
	virtual void update(float pDelta) {}
	virtual void postupdate(float pDelta) {}

protected:
	virtual json on_serialize(serialize_type) { return{}; }
	virtual void on_deserialize(const json&) {}

private:
	std::reference_wrapper<layer> mLayer;
};

} // namespace wge::core
