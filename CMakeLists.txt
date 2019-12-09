cmake_minimum_required(VERSION 3.0)

project(WolfGangEngine)

set(WGE_HEADER_DIR "${CMAKE_SOURCE_DIR}/include/wge")
set(WGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(WGE_3RDPARTY_DIR "${CMAKE_SOURCE_DIR}/3rdparty")

# Use latest standard in Visual Studio
if (MSVC_VERSION GREATER_EQUAL "1900")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
	if (_cpp_latest_flag_supported)
		add_compile_options("/std:c++latest")
	endif()
endif()

# Use the angelscript namespace
# Disabled for now because of some minor errors
# add_definitions(-DAS_USE_NAMESPACE)

# Tell ImGui that we are using GLEW
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

include_directories("${CMAKE_SOURCE_DIR}/include")

# GLOBing for now

file(GLOB WGE_AUDIO_HEADERS "${WGE_HEADER_DIR}/audio/*.hpp")
source_group("Audio" FILES ${WGE_AUDIO_HEADERS})

file(GLOB WGE_MATH_HEADERS "${WGE_HEADER_DIR}/math/*.hpp")
source_group("Math" FILES ${WGE_MATH_HEADERS})

file(GLOB WGE_GRAPHICS_HEADERS "${WGE_HEADER_DIR}/graphics/*.hpp")
source_group("Graphics" FILES ${WGE_GRAPHICS_HEADERS})

file(GLOB WGE_PHYSICS_HEADERS "${WGE_HEADER_DIR}/physics/*.hpp")
source_group("Physics" FILES ${WGE_PHYSICS_HEADERS})

file(GLOB WGE_SCRIPTING_HEADERS "${WGE_HEADER_DIR}/scripting/*.hpp")
source_group("Scripting" FILES ${WGE_SCRIPTING_HEADERS})

file(GLOB WGE_CORE_HEADERS "${WGE_HEADER_DIR}/core/*.hpp")
source_group("Core" FILES ${WGE_CORE_HEADERS})

file(GLOB WGE_LOGGING_HEADERS "${WGE_HEADER_DIR}/logging/*.hpp")
source_group("Logging" FILES ${WGE_LOGGING_HEADERS})

file(GLOB WGE_FILESYSTEM_HEADERS "${WGE_HEADER_DIR}/filesystem/*.hpp")
source_group("Filesystem" FILES ${WGE_FILESYSTEM_HEADERS})

file(GLOB WGE_UTIL_HEADERS "${WGE_HEADER_DIR}/util/*.hpp")
source_group("Util" FILES ${WGE_UTIL_HEADERS})

file(GLOB WGE_EDITOR_HEADERS "${WGE_HEADER_DIR}/editor/*.hpp")
file(GLOB_RECURSE WGE_EDITOR_HEADERS "${WGE_SOURCE_DIR}/editor/*.hpp")
source_group("Editor" FILES ${WGE_EDITOR_HEADERS})

set(WGE_ALL_HEADERS
	${WGE_AUDIO_HEADERS}
	${WGE_MATH_HEADERS}
	${WGE_GRAPHICS_HEADERS}
	${WGE_PHYSICS_HEADERS}
	${WGE_SCRIPTING_HEADERS}
	${WGE_CORE_HEADERS}
	${WGE_LOGGING_HEADERS}
	${WGE_FILESYSTEM_HEADERS}
	${WGE_UTIL_HEADERS}
	)
	
file(GLOB WGE_AUDIO_SOURCES "${WGE_SOURCE_DIR}/audio/*.cpp")
source_group("Audio" FILES ${WGE_AUDIO_SOURCES})

file(GLOB WGE_MATH_SOURCES "${WGE_SOURCE_DIR}/math/*.cpp")
source_group("Math" FILES ${WGE_MATH_SOURCES})

file(GLOB WGE_GRAPHICS_SOURCES "${WGE_SOURCE_DIR}/graphics/*.cpp")
source_group("Graphics" FILES ${WGE_GRAPHICS_SOURCES})

file(GLOB WGE_PHYSICS_SOURCES "${WGE_SOURCE_DIR}/physics/*.cpp")
source_group("Physics" FILES ${WGE_PHYSICS_SOURCES})

file(GLOB WGE_SCRIPTING_SOURCES "${WGE_SOURCE_DIR}/scripting/*.cpp")
source_group("Scripting" FILES ${WGE_SCRIPTING_SOURCES})

file(GLOB WGE_CORE_SOURCES "${WGE_SOURCE_DIR}/core/*.cpp")
source_group("Core" FILES ${WGE_CORE_SOURCES})

file(GLOB WGE_LOGGING_SOURCES "${WGE_SOURCE_DIR}/logging/*.cpp")
source_group("Logging" FILES ${WGE_LOGGING_SOURCES})

file(GLOB WGE_FILESYSTEM_SOURCES "${WGE_SOURCE_DIR}/filesystem/*.cpp")
source_group("Filesystem" FILES ${WGE_FILESYSTEM_SOURCES})

file(GLOB WGE_UTIL_SOURCES "${WGE_SOURCE_DIR}/util/*.cpp")
source_group("Util" FILES ${WGE_UTIL_SOURCES})

file(GLOB_RECURSE WGE_EDITOR_SOURCES "${WGE_SOURCE_DIR}/editor/*.cpp")
source_group("Editor" FILES ${WGE_EDITOR_SOURCES})

set(WGE_ALL_SOURCES
	${WGE_AUDIO_SOURCES}
	${WGE_MATH_SOURCES}
	${WGE_GRAPHICS_SOURCES}
	${WGE_PHYSICS_SOURCES}
	${WGE_SCRIPTING_SOURCES}
	${WGE_CORE_SOURCES}
	${WGE_LOGGING_SOURCES}
	${WGE_FILESYSTEM_SOURCES}
	${WGE_UTIL_SOURCES}
	)

# ImGUI
set(IMGUI_INCLUDE_DIR "${WGE_3RDPARTY_DIR}/imgui/include")
include_directories(${IMGUI_INCLUDE_DIR})
set(IMGUI_HEADERS
	"${IMGUI_INCLUDE_DIR}/imgui/imgui.h"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_stl.h"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_glfw.h"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_opengl3.h"
	)
set(IMGUI_SOURCES
	"${IMGUI_INCLUDE_DIR}/imgui/imgui.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_widgets.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_stl.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_draw.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_style_editor.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_glfw.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_opengl3.cpp"
	)
source_group("ImGui Headers" FILES ${IMGUI_HEADERS})
source_group("ImGui Sources" FILES ${IMGUI_SOURCES})

# STB
set(STB_HEADERS
	"${WGE_3RDPARTY_DIR}/stb/include/stb/stb_vorbis.h"
	)
set(STB_SOURCES
	"${WGE_3RDPARTY_DIR}/stb/include/stb/stb_vorbis.c"
	"${WGE_3RDPARTY_DIR}/stb/include/stb/impl.c"
	)
source_group("STB Headers" FILES ${STB_HEADERS})
source_group("STB Sources" FILES ${STB_SOURCES})

# AngelScript Add-ons
set(ANGELSCRIPT_ADDONS
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptbuilder/scriptbuilder.h"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scripthandle/scripthandle.h"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scripthandle/scripthandle.cpp"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring.h"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptarray/scriptarray.h"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/scriptarray/scriptarray.cpp"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/contextmgr/contextmgr.h"
	"${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on/contextmgr/contextmgr.cpp"
	)
include_directories("${WGE_3RDPARTY_DIR}/AngelScript/sdk/add_on")
source_group("AngelScript Addons" FILES ${ANGELSCRIPT_ADDONS})

# Box2D
file(GLOB_RECURSE BOX2D_SOURCES "${WGE_3RDPARTY_DIR}/box2d/Box2D/*.cpp")
file(GLOB_RECURSE BOX2D_HEADERS "${WGE_3RDPARTY_DIR}/box2d/Box2D/*.h")
source_group("Box2d Sources" FILES ${BOX2D_SOURCES})
source_group("Box2d Headers" FILES ${BOX2D_HEADERS})
include_directories("${WGE_3RDPARTY_DIR}/box2d")

# Lua
file(GLOB_RECURSE LUA_SOURCES "${WGE_3RDPARTY_DIR}/lua/*.c")
file(GLOB_RECURSE LUA_HEADERS "${WGE_3RDPARTY_DIR}/lua/*.h")
source_group("Lua Sources" FILES ${LUA_SOURCES})
source_group("Lua Headers" FILES ${LUA_HEADERS})
include_directories("${WGE_3RDPARTY_DIR}/lua")

# sol
file(GLOB_RECURSE SOL_HEADERS "${WGE_3RDPARTY_DIR}/sol/include/sol/*.hpp")
source_group("Sol Headers" FILES ${SOL_HEADERS})
include_directories("${WGE_3RDPARTY_DIR}/sol/include")

# fmt
file(GLOB_RECURSE FMT_HEADERS "${WGE_3RDPARTY_DIR}/fmt/include/fmt/*.h")
file(GLOB_RECURSE FMT_SOURCES "${WGE_3RDPARTY_DIR}/fmt/src/*.cc")
source_group("fmt" FILES ${FMT_SOURCES} ${FMT_HEADERS})
include_directories("${WGE_3RDPARTY_DIR}/fmt/include")

# Combine all the sources into one package :)
set(ALL_SOURCES
	${STB_HEADERS}
	${STB_SOURCES}
	${BOX2D_HEADERS}
	${BOX2D_SOURCES}
	${LUA_SOURCES}
	${LUA_HEADERS}
	${SOL_HEADERS}
	${FMT_SOURCES}
	${FMT_HEADERS}
	${ANGELSCRIPT_ADDONS}
	${WGE_ALL_HEADERS}
	${WGE_ALL_SOURCES}
	)

# Set output to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# The main engine will be a library
add_library(WolfGangEngine ${ALL_SOURCES})

# Note: Not using this for now but keeping it for the future.
#
# Copy resources to executable location every build.
# We'll use the POST_BUILD option so it will only
# copy files if the build succeeds.
#add_custom_command(TARGET WolfGangEngine POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E copy_directory
#                       ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:WolfGangEngine>)

# GLEW
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory("${WGE_3RDPARTY_DIR}/glfw")
include_directories("${WGE_3RDPARTY_DIR}/glfw/include")
target_link_libraries(WolfGangEngine glfw ${GLFW_LIBRARIES})

# AngelScript
add_subdirectory("${WGE_3RDPARTY_DIR}/angelscript/sdk/angelscript/projects/cmake")
include_directories("${WGE_3RDPARTY_DIR}/angelscript/sdk/angelscript/include")
target_link_libraries(WolfGangEngine ${ANGELSCRIPT_LIBRARY_NAME})

# OpenGL
find_package(OpenGL)
target_link_libraries(WolfGangEngine ${OPENGL_gl_LIBRARY})

# GLEW
add_subdirectory("${WGE_3RDPARTY_DIR}/glew/build/cmake")
include_directories("${WGE_3RDPARTY_DIR}/glew/include")
target_link_libraries(WolfGangEngine glew_s)

# JSON
include_directories("${WGE_3RDPARTY_DIR}/json/include")

# OpenAL-Soft
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_TESTS OFF)
set(ALSOFT_UTILS OFF)
set(ALSOFT_CONFIG OFF)
add_subdirectory("${WGE_3RDPARTY_DIR}/openal-soft")
include_directories("${OpenAL_SOURCE_DIR}/include")
target_link_libraries(WolfGangEngine OpenAL)

# STB Nothings
include_directories("${WGE_3RDPARTY_DIR}/stb/include")

# Editor project
add_executable(WgeEditor
	"${WGE_SOURCE_DIR}/main.cpp" 
	${IMGUI_HEADERS}
	${IMGUI_SOURCES}
	${WGE_EDITOR_HEADERS}
	${WGE_EDITOR_SOURCES}
	)
target_link_libraries(WgeEditor WolfGangEngine)

# Test project
file(GLOB_RECURSE WGE_TESTS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(WgeTests ${WGE_TESTS})
include_directories("${WGE_3RDPARTY_DIR}/catch2/single_include")
target_link_libraries(WgeTests WolfGangEngine)

# Set the VS debugger's working directory
set_property(TARGET WgeEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources)
set_property(TARGET WgeTests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources)
