
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
	const auto filepath = get_location().get_autonamed_file(".lua");
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
	const auto filepath = get_location().get_autonamed_file(".lua");
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

} // namespace wge::scripting
