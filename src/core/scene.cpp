#include <wge/core/scene.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/engine.hpp>
#include <wge/filesystem/file_input_stream.hpp>

namespace wge::core
{

layer* scene::get_layer(std::size_t pIndex)
{
	if (pIndex >= mLayers.size())
		return{};
	return mLayers[pIndex].get();
}

layer* scene::add_layer()
{
	return mLayers.emplace_back(util::make_copyable_ptr<layer>()).get();
}

layer* scene::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
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

scene::layers& scene::get_layer_container() noexcept
{
	return mLayers;
}

void scene::clear()
{
	mLayers.clear();
}

} // namespace wge::core
