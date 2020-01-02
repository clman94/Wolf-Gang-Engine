#include <wge/core/scene.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/engine.hpp>
#include <wge/filesystem/file_input_stream.hpp>

#include <iterator>

namespace wge::core
{

layer* scene::get_layer(std::size_t pIndex)
{
	if (pIndex >= mLayers.size())
		return{};
	auto iter = mLayers.begin();
	std::advance(iter, pIndex);
	return &(*iter);
}

layer* scene::add_layer()
{
	return &mLayers.emplace_back();
}

layer* scene::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
}

bool scene::remove_layer(const layer& pLayer)
{
	for (auto i = mLayers.begin(); i != mLayers.end(); i++)
	{
		if (&*i == &pLayer)
		{
			mLayers.erase(i);
			return true;
		}
	}
	return false;
}

scene::layers& scene::get_layer_container() noexcept
{
	return mLayers;
}

void scene::clear()
{
	mLayers.clear();
}

} // namespace wge::core
