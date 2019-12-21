#include <wge/core/scene.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/engine.hpp>
#include <wge/filesystem/file_input_stream.hpp>

namespace wge::core
{

layer* scene::get_layer(std::size_t pIndex) const
{
	if (pIndex >= mLayers.size())
		return{};
	return mLayers[pIndex].get();
}

layer* scene::add_layer()
{
	return mLayers.emplace_back(layer::create(*mFactory)).get();
}

layer* scene::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
}

layer* scene::add_layer(const std::string& pName, std::size_t pInsert)
{
	return mLayers.insert(mLayers.begin() + pInsert, layer::create(*mFactory))->get();
}

layer* scene::add_layer(layer::uptr pPtr)
{
	return mLayers.emplace_back(std::move(pPtr)).get();
}

layer::uptr scene::release_layer(const layer& pLayer)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (mLayers[i].get() == &pLayer)
		{
			auto temp = std::move(mLayers[i]);
			mLayers.erase(mLayers.begin() + i);
			return temp;
		}
	}
	return{};
}

bool scene::remove_layer(const layer& pLayer)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (mLayers[i].get() == &pLayer)
		{
			mLayers.erase(mLayers.begin() + i);
			return true;
		}
	}
	return false;
}

const scene::layers& scene::get_layer_container() const noexcept
{
	return mLayers;
}

const factory& scene::get_factory() const noexcept
{
	return *mFactory;
}

void scene::clear()
{
	mLayers.clear();
}

} // namespace wge::core
