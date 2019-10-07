#pragma once

#include <wge/core/layer.hpp>
#include <wge/core/factory.hpp>

#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <tuple>

namespace wge::core
{

class scene
{
public:
	using uptr = std::unique_ptr<scene>;
	using layers = std::vector<layer::uptr>;

	scene(const factory& pFactory) :
		mFactory(&pFactory)
	{}

	// Get a layer by index
	[[nodiscard]] layer* get_layer(std::size_t pIndex) const;

	layer* add_layer();
	layer* add_layer(const std::string& pName);
	layer* add_layer(const std::string& pName, std::size_t pInsert);
	layer* add_layer(layer::uptr);
	layer::uptr release_layer(const layer&);

	// Returns true if the layer was successfully removed.
	bool remove_layer(const layer& pPtr);

	const layers& get_layer_container() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

	void clear();

	// For each system T, call pCallable.
	// Tcallable should be void(layer&, T&)
	template <typename T, typename Tcallable>
	void for_each_system(Tcallable&& pCallable);

	const factory& get_factory() const noexcept;

private:
	layers mLayers;
	const factory* mFactory;
};

template<typename T, typename Tcallable>
inline void scene::for_each_system(Tcallable&& pCallable)
{
	for (auto& i : mLayers)
		if (auto sys = i->get_system<T>())
			pCallable(*i.get(), *sys);
}

} // namespace wge::core
