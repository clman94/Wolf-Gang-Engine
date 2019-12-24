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
	return &mLayers[pIndex];
}

layer* scene::add_layer()
{
	if (mFactory) // Optional factory injection.
		return &mLayers.emplace_back(*mFactory);
	else
		return &mLayers.emplace_back();
}

layer* scene::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
}

layer* scene::add_layer(const std::string& pName, std::size_t pInsert)
{
	return &(*mLayers.insert(mLayers.begin() + pInsert,
		// Optional factory injection.
		mFactory ? layer{ *mFactory } : layer{}));
}

bool scene::remove_layer(const layer& pLayer)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (&mLayers[i] == &pLayer)
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

const factory& scene::get_factory() const noexcept
{
	return *mFactory;
}

void scene::clear()
{
	mLayers.clear();
}

} // namespace wge::core
