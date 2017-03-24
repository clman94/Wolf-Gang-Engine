// Simply defines the filesysten header from C++ 17 or boost

#ifndef ENGINE_FILESYSTEM_HPP
#define ENGINE_FILESYSTEM_HPP

#if defined(__MINGW32__) || defined(__GNUC__)
#include <boost/filesystem.hpp>
namespace engine {
namespace fs = boost::filesystem;
}
#else
#include <filesystem>
namespace engine {
namespace fs = std::experimental::filesystem;
}
#endif
#endif