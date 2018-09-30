#include <wge/graphics/texture_asset_loader.hpp>
#include <wge/graphics/texture.hpp>

#include <iostream>
#include <fstream>

using namespace wge;
using namespace wge::graphics;

core::asset::ptr texture_asset_loader::create_asset(core::asset_config::ptr pConfig, const filesystem::path & mRoot_path)
{
	filesystem::path tex_path = pConfig->get_path();
	tex_path.remove_extension(); // "file.png.asset" -> "file.png"

	texture::ptr tex = std::make_shared<texture>(pConfig);
	tex->set_path(make_path_relative(tex_path, mRoot_path));
	tex->load(tex_path.string());
	return tex;
}

bool texture_asset_loader::can_import(const filesystem::path & pPath)
{
	return pPath.extension() == ".png";
}

core::asset::ptr texture_asset_loader::import_asset(const filesystem::path & pPath, const filesystem::path & mRoot_path)
{
	core::asset_config::ptr config = std::make_shared<core::asset_config>();
	config->set_type("texture");

	// Generate an id from a hash of the file path
	// TODO: Replace this with something that can generate a more "unique" id.
	config->set_id(util::hash::hash64(pPath.string()));

	filesystem::path config_path(pPath);
	config_path.pop_filepath();
	config_path /= pPath.filename() + ".asset";
	config->set_path(config_path);

	// Save the configuration
	std::ofstream out_config_stream(config_path.string().c_str());
	out_config_stream << config->save().dump(2);

	return create_asset(config, mRoot_path);
}
