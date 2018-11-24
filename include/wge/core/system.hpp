#pragma once

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
	system(layer& pLayer) :
		mLayer(pLayer)
	{}
	virtual ~system() {}
	virtual int get_system_id() const = 0;
	virtual std::string get_system_name() const = 0;

	layer& get_layer() const
	{
		return mLayer;
	}

	virtual void preupdate(float pDelta) {}
	virtual void update(float pDelta) {}
	virtual void postupdate(float pDelta) {}

private:
	std::reference_wrapper<layer> mLayer;
};

} // namespace wge::core
