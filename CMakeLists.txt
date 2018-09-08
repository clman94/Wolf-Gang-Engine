cmake_minimum_required(VERSION 3.0)

project(WolfGangEngine)

# Use latest standard in Visual Studio
if (MSVC_VERSION GREATER_EQUAL "1900")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
	if (_cpp_latest_flag_supported)
		add_compile_options("/std:c++latest")
	endif()
endif()


include_directories("include")

# GLOBing for now

file(GLOB WGE_AUDIO_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/audio/*.hpp")
source_group("Audio" FILES ${WGE_AUDIO_HEADERS})

file(GLOB WGE_MATH_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/math/*.hpp")
source_group("Math" FILES ${WGE_MATH_HEADERS})

file(GLOB WGE_GRAPHICS_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/graphics/*.hpp")
source_group("Graphics" FILES ${WGE_GRAPHICS_HEADERS})

file(GLOB WGE_PHYSICS_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/physics/*.hpp")
source_group("Physics" FILES ${WGE_PHYSICS_HEADERS})

file(GLOB WGE_CORE_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/core/*.hpp")
source_group("Core" FILES ${WGE_CORE_HEADERS})

file(GLOB WGE_FILESYSTEM_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/filesystem/*.hpp")
source_group("Filesystem" FILES ${WGE_FILESYSTEM_HEADERS})

file(GLOB WGE_UTIL_HEADERS "${CMAKE_SOURCE_DIR}/include/wge/util/*.hpp")
source_group("Util" FILES ${WGE_UTIL_HEADERS})

set(WGE_ALL_HEADERS
	${WGE_AUDIO_HEADERS}
	${WGE_MATH_HEADERS}
	${WGE_GRAPHICS_HEADERS}
	${WGE_PHYSICS_HEADERS}
	${WGE_CORE_HEADERS}
	${WGE_FILESYSTEM_HEADERS}
	${WGE_UTIL_HEADERS}
	)

file(GLOB WGE_AUDIO_SOURCES "${CMAKE_SOURCE_DIR}/src/audio/*.cpp")
source_group("Audio" FILES ${WGE_AUDIO_SOURCES})

file(GLOB WGE_MATH_SOURCES "${CMAKE_SOURCE_DIR}/src/math/*.cpp")
source_group("Math" FILES ${WGE_MATH_SOURCES})

file(GLOB WGE_GRAPHICS_SOURCES "${CMAKE_SOURCE_DIR}/src/graphics/*.cpp")
source_group("Graphics" FILES ${WGE_GRAPHICS_SOURCES})

file(GLOB WGE_PHYSICS_SOURCES "${CMAKE_SOURCE_DIR}/src/physics/*.cpp")
source_group("Physics" FILES ${WGE_PHYSICS_SOURCES})

file(GLOB WGE_CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
source_group("Core" FILES ${WGE_CORE_SOURCES})

file(GLOB WGE_FILESYSTEM_SOURCES "${CMAKE_SOURCE_DIR}/src/filesystem/*.cpp")
source_group("Filesystem" FILES ${WGE_FILESYSTEM_SOURCES})

file(GLOB WGE_UTIL_SOURCES "${CMAKE_SOURCE_DIR}/src/util/*.cpp")
source_group("Util" FILES ${WGE_UTIL_SOURCES})

set(WGE_ALL_SOURCES
	${WGE_AUDIO_SOURCES}
	${WGE_MATH_SOURCES}
	${WGE_GRAPHICS_SOURCES}
	${WGE_PHYSICS_SOURCES}
	${WGE_CORE_SOURCES}
	${WGE_FILESYSTEM_SOURCES}
	${WGE_UTIL_SOURCES}
	)

# ImGUI
set(IMGUI_INCLUDE_DIR "3rdparty/imgui/include")
include_directories(${IMGUI_INCLUDE_DIR})
set(IMGUI_HEADERS
	"${IMGUI_INCLUDE_DIR}/imgui/imgui.h"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_glfw.h"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_opengl3.h"
	)
set(IMGUI_SOURCES
	"${IMGUI_INCLUDE_DIR}/imgui/imgui.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_draw.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_style_editor.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_glfw.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui/imgui_impl_opengl3.cpp"
	)
source_group("ImGui Headers" FILES ${IMGUI_HEADERS})
source_group("ImGui Sources" FILES ${IMGUI_SOURCES})

# STB
set(STB_HEADERS
	"3rdparty/stb/include/stb/stb_vorbis.h"
	)
set(STB_SOURCES
	"3rdparty/stb/include/stb/stb_vorbis.c"
	"3rdparty/stb/include/stb/impl.c"
	)
source_group("STB Headers" FILES ${STB_HEADERS})
source_group("STB Sources" FILES ${STB_SOURCES})

# AngelScript Add-ons
set(ANGELSCRIPT_ADDONS
	"3rdparty/AngelScript/sdk/add_on/scriptbuilder/scriptbuilder.h"
	"3rdparty/AngelScript/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
	"3rdparty/AngelScript/sdk/add_on/scripthandle/scripthandle.h"
	"3rdparty/AngelScript/sdk/add_on/scripthandle/scripthandle.cpp"
	"3rdparty/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring.h"
	"3rdparty/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
	"3rdparty/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp"
	"3rdparty/AngelScript/sdk/add_on/scriptarray/scriptarray.h"
	"3rdparty/AngelScript/sdk/add_on/scriptarray/scriptarray.cpp"
	"3rdparty/AngelScript/sdk/add_on/contextmgr/contextmgr.h"
	"3rdparty/AngelScript/sdk/add_on/contextmgr/contextmgr.cpp"
	)
include_directories("3rdparty/AngelScript/sdk/add_on")
source_group("AngelScript Addons" FILES ${ANGELSCRIPT_ADDONS})

# Box2D
file(GLOB_RECURSE BOX2D_SOURCES "3rdparty/box2d/Box2D/*.cpp")
file(GLOB_RECURSE BOX2D_HEADERS "3rdparty/box2d/Box2D/*.h")
source_group("Box2d Sources" FILES ${BOX2D_SOURCES})
source_group("Box2d Headers" FILES ${BOX2D_HEADERS})
include_directories("3rdparty/box2d")

# Combine all the sources into one package :)
set(ALL_SOURCES
	${STB_HEADERS}
	${STB_SOURCES}
	${IMGUI_HEADERS}
	${IMGUI_SOURCES}
	${BOX2D_HEADERS}
	${BOX2D_SOURCES}
	${ANGELSCRIPT_ADDONS}
	${WGE_ALL_HEADERS}
	${WGE_ALL_SOURCES}
	)

# Set output to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(WolfGangEngine main.cpp ${ALL_SOURCES})

# Set the VS debugger's working directory to the binaries
set_property(TARGET WolfGangEngine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources)

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
add_subdirectory("3rdparty/glfw")
include_directories("3rdparty/glfw/include")
target_link_libraries(WolfGangEngine glfw ${GLFW_LIBRARIES})

# AngelScript
add_subdirectory("3rdparty/angelscript/sdk/angelscript/projects/cmake")
include_directories("3rdparty/angelscript/sdk/angelscript/include")
target_link_libraries(WolfGangEngine ${ANGELSCRIPT_LIBRARY_NAME})

# OpenGL
find_package(OpenGL)
target_link_libraries(WolfGangEngine ${OPENGL_gl_LIBRARY})

# GLEW
add_subdirectory("3rdparty/glew/build/cmake")
include_directories("3rdparty/glew/include")
target_link_libraries(WolfGangEngine glew_s)

# JSON
include_directories("3rdparty/json/include")

# OpenAL-Soft
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_TESTS OFF)
set(ALSOFT_UTILS OFF)
set(ALSOFT_CONFIG OFF)
add_subdirectory("3rdparty/OpenAL-Soft")
include_directories("${OpenAL_SOURCE_DIR}/include")
target_link_libraries(WolfGangEngine OpenAL)

# STB Nothings
include_directories("3rdparty/stb/include")
