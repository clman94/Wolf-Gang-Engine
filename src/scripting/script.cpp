
#include <wge/scripting/script.hpp>
#include <wge/logging/log.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>

#include <regex>
#include <fstream>

namespace wge::scripting
{

void script::parse_function_list()
{
	function_list.clear();

	std::regex func_reg("function\\s+(.+)\\s*\\(");
	auto begin_iter = std::sregex_iterator(source.begin(), source.end(), func_reg);
	auto end_iter = std::sregex_iterator();
	int line = 1;
	for (auto i = begin_iter; i != end_iter; ++i)
	{
		for (auto c = i->prefix().first; c != i->prefix().second; c++)
			if (*c == '\n')
				++line;
		function_list.push_back({ line, (*i)[1] });
	}
}

void script::load()
{
	const auto filepath = get_location()->get_autonamed_file(".lua");
	try
	{
		std::ifstream stream(filepath.string().c_str());
		std::stringstream sstr;
		sstr << stream.rdbuf();
		source = sstr.str();
	}
	catch (const std::exception& e)
	{
		log::error("Couldn't load resource from path \"{}\"", filepath.string());
		log::error("Exception: {}", e.what());
	}
}

void script::save()
{
	const auto filepath = get_location()->get_autonamed_file(".lua");
	try
	{
		filesystem::file_stream stream;
		stream.open(filepath, filesystem::stream_access::write);
		stream.write(source);
	}
	catch (const filesystem::io_error& e)
	{
		log::error("Couldn't save resource to path \"{}\"", filepath.string());
		log::error("Exception: {}", e.what());
	}
}

script::handle script::create_secondary_asset(const core::asset::ptr& pParent, const std::string& pName, const std::string& pDefault_text)
{
	try {
		// Generate a file.
		filesystem::file_stream out;
		out.open(pParent->get_location()->get_file(std::string(pName) + ".lua"), filesystem::stream_access::write);
		out.write(pDefault_text);
		out.close();
	}
	catch (const filesystem::io_error& e)
	{
		log::error("Couldn't generate file for event.");
		log::error("io_error: {}", e.what());
		return nullptr;
	}
	return load_secondary_asset(pParent, pName, util::generate_uuid());
}

script::handle script::load_secondary_asset(const core::asset::ptr& pParent, const std::string& pName, const core::asset_id& pId)
{
	const auto new_asset = std::make_shared<core::asset>();
	new_asset->set_name(pName);
	new_asset->set_id(pId);
	new_asset->set_parent_id(pParent->get_id());

	auto script_resource = std::make_unique<scripting::script>();
	const auto primary_location = std::dynamic_pointer_cast<core::primary_asset_location>(pParent->get_location());
	script_resource->set_location(core::secondary_asset_location::create(primary_location, pName));
	script_resource->load();
	new_asset->set_resource(std::move(script_resource));

	return new_asset;
}

} // namespace wge::scripting
