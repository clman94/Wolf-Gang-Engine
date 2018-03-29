cmake_minimum_required(VERSION 3.0.2)

project(WolfGangEngine)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static libs
set(CompilerFlags
	CMAKE_CXX_FLAGS
	CMAKE_CXX_FLAGS_DEBUG
	CMAKE_CXX_FLAGS_RELEASE
	CMAKE_C_FLAGS
	CMAKE_C_FLAGS_DEBUG
	CMAKE_C_FLAGS_RELEASE
	)
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

# Use latest standard in Visual Studio
if (MSVC_VERSION GREATER_EQUAL "1900")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
	if (_cpp_latest_flag_supported)
		add_compile_options("/std:c++latest")
	endif()
endif()

# Main (editor) sources
set(MAIN_SOURCES "${CMAKE_SOURCE_DIR}/src/main/main.cpp")

# Locked sources
set(LOCKED_MAIN_SOURCES "${CMAKE_SOURCE_DIR}/src/main/main.cpp")

# Test sources
set(TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/tests1.cpp")

# Engine sources and Headers
file(GLOB_RECURSE ENGINE_RPG_SOURCES "${CMAKE_SOURCE_DIR}/src/rpg/*.cpp")
file(GLOB_RECURSE ENGINE_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/engine/*.cpp")
file(GLOB_RECURSE ENGINE_UI_SOURCES "${CMAKE_SOURCE_DIR}/src/ui/*.cpp")
set(ENGINE_SOURCES ${ENGINE_RPG_SOURCES} ${ENGINE_ENGINE_SOURCES} ${ENGINE_UI_SOURCES})

file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_SOURCE_DIR}/include/*.hpp")

# Box2d sources and Headers
file(GLOB_RECURSE BOX2D_SOURCES "${CMAKE_SOURCE_DIR}/3rdparty/box2d/Box2D/Box2D/*.cpp")
file(GLOB_RECURSE BOX2D_HEADERS "${CMAKE_SOURCE_DIR}/3rdparty/box2d/Box2D/Box2D/*.h")

# Tinyxml2 sources and Headers
set(TINYXML2_SOURCES "${CMAKE_SOURCE_DIR}/3rdparty/tinyxml2/tinyxml2.cpp")
set(TINYXML2_HEADERS "${CMAKE_SOURCE_DIR}/3rdparty/tinyxml2/tinyxml2.h")

set(IMGUI_SOURCES
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui.cpp"
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui_draw.cpp"
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-SFML.cpp"
	)
set(IMGUI_HEADERS
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui.h"
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-SFML.h"
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imconfig-SFML.h"
	"${CMAKE_SOURCE_DIR}/3rdparty/imgui/imconfig.h"
	)

# Angelscript Addon sources and Headers
set(AS_ADDONS "${CMAKE_SOURCE_DIR}/3rdparty/AngelScript/sdk/add_on")
set(AS_ADDONS_SOURCES
	"${AS_ADDONS}/scriptbuilder/scriptbuilder.cpp"
	"${AS_ADDONS}/scriptdictionary/scriptdictionary.cpp"
	"${AS_ADDONS}/scripthandle/scripthandle.cpp"
	"${AS_ADDONS}/scriptstdstring/scriptstdstring.cpp"
	"${AS_ADDONS}/scriptarray/scriptarray.cpp"
	"${AS_ADDONS}/scriptdictionary/scriptdictionary.cpp"
	"${AS_ADDONS}/scriptmath/scriptmath.cpp"
	)
set(AS_ADDONS_HEADERS
	"${AS_ADDONS}/scriptbuilder/scriptbuilder.h"
	"${AS_ADDONS}/scriptdictionary/scriptdictionary.h"
	"${AS_ADDONS}/scripthandle/scripthandle.h"
	"${AS_ADDONS}/scriptstdstring/scriptstdstring.h"
	"${AS_ADDONS}/scriptarray/scriptarray.h"
	"${AS_ADDONS}/scriptdictionary/scriptdictionary.h"
	"${AS_ADDONS}/scriptmath/scriptmath.h"
	)


# Organize into little folders for ease of use
source_group("Engine Sources" FILES ${ENGINE_SOURCES})
source_group("Engine Headers" FILES ${ENGINE_HEADERS})

source_group("Box2d Sources" FILES ${BOX2D_SOURCES})
source_group("Box2d Headers" FILES ${BOX2D_HEADERS})

source_group("Tinyxml2 Sources" FILES ${TINYXML2_SOURCES})
source_group("Tinyxml2 Headers" FILES ${TINYXML2_HEADERS})

source_group("Angelscript Addons Sources" FILES ${AS_ADDONS_SOURCES})
source_group("Angelscript Addons Headers" FILES ${AS_ADDONS_HEADERS})

source_group("ImGui Sources" FILES ${IMGUI_SOURCES})
source_group("ImGui Headers" FILES ${IMGUI_HEADERS})

source_group("Main Sources" FILES ${MAIN_SOURCES} ${LOCKED_MAIN_SOURCES} ${TEST_SOURCES})

# TODO: Add subdirectories instead
set(WGE_ALL_SOURCES
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
	${BOX2D_SOURCES}
	${BOX2D_HEADERS}
	${TINYXML2_SOURCES}
	${TINYXML2_HEADERS}
	${AS_ADDONS_SOURCES}
	${AS_ADDONS_HEADERS}
	${IMGUI_SOURCES}
	${IMGUI_HEADERS}
	)

# Setup executables with appropriate sources and headers
add_executable(WolfGangEngine        ${MAIN_SOURCES}        ${WGE_ALL_SOURCES})
add_executable(WolfGangEngine_Locked ${LOCKED_MAIN_SOURCES} ${WGE_ALL_SOURCES})
add_executable(WolfGangEngine_Tests  ${TEST_SOURCES}        ${WGE_ALL_SOURCES})

# Set the locked release mode for the WolfGangEngine_Locked target
target_compile_definitions(WolfGangEngine_Locked PRIVATE LOCKED_RELEASE_MODE=1)

# WGE includes
include_directories("${CMAKE_SOURCE_DIR}/include")

# Box2d includes
include_directories("${CMAKE_SOURCE_DIR}/3rdparty/box2d/Box2D")

# imgui includes
include_directories("${CMAKE_SOURCE_DIR}/3rdparty/imgui")

# SFML
set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/sfml")

# Link to sfml dependencies (Stolen from tgui)
include(${CMAKE_SOURCE_DIR}/3rdparty/sfml/cmake/Config.cmake)
if (SFML_OS_WINDOWS)
	set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "${SFML_ROOT}/extlibs/headers")
	if(ARCH_32BITS)
		if(SFML_COMPILER_MSVC AND MSVC_VERSION LESS 1900) # older than VC++14
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-msvc/x86")
		elseif(SFML_COMPILER_MSVC) # VC++14 or newer
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-msvc-universal/x86")
		else() # gcc
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-mingw/x86")
		endif()
	elseif(ARCH_64BITS)
		if(SFML_COMPILER_MSVC AND MSVC_VERSION LESS 1900) # older than VC++14
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-msvc/x64")
		elseif(SFML_COMPILER_MSVC) # VC++14 or newer
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-msvc-universal/x64")
		else() # gcc
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-mingw/x64")
		endif()
	endif()
elseif(SFML_OS_MACOSX)
	set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "${SFML_ROOT}/extlibs/headers")
	set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-osx/lib/")
elseif(SFML_OS_ANDROID)
	set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${ANDROID_NDK}/sources/sfml/extlibs/lib/armeabi/")
elseif(SFML_OS_IOS)
	set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${SFML_ROOT}/extlibs/libs-ios/")
endif()

# Do all the modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# Find SFML
find_package(SFML 2 COMPONENTS audio graphics window system)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})

  # Link SFML
  target_link_libraries(WolfGangEngine        ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
  target_link_libraries(WolfGangEngine_Locked ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
  target_link_libraries(WolfGangEngine_Tests  ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})

endif()

# Find TGUI
set(TGUI_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/tgui")
find_package(TGUI 0.7)
if(TGUI_FOUND)
  include_directories(${TGUI_INCLUDE_DIR})

  # Link TGUI
  target_link_libraries(WolfGangEngine ${TGUI_LIBRARY})
  target_link_libraries(WolfGangEngine_Locked ${TGUI_LIBRARY})
  target_link_libraries(WolfGangEngine_Tests ${TGUI_LIBRARY})

endif()

# Use namespaces in AngelScript
add_definitions(-DAS_USE_NAMESPACE)

# Angelscript Includes
include_directories("${CMAKE_SOURCE_DIR}/3rdparty/AngelScript/sdk/angelscript/include")

# Set Angelscript root folder (libs and include here)
set(AS_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/AngelScript/sdk/angelscript")

# Find the static angelscript debug library
find_library(AS_LIBRARY_DEBUG
	NAMES angelscriptd
	PATH_SUFFIXES lib build/lib build/lib/Debug
	PATHS ${AS_ROOT}
	)

# Find the static angelscript release library
find_library(AS_LIBRARY_RELEASE
	NAMES angelscript
	PATH_SUFFIXES lib build/lib build/lib/Release
	PATHS ${AS_ROOT}
	)

set(AS_LINK_LIBRARIES
	debug     ${AS_LIBRARY_DEBUG}
	optimized ${AS_LIBRARY_RELEASE}
	)
	
# Link AngelScript
target_link_libraries(WolfGangEngine ${AS_LINK_LIBRARIES})
target_link_libraries(WolfGangEngine_Locked ${AS_LINK_LIBRARIES})
target_link_libraries(WolfGangEngine_Tests ${AS_LINK_LIBRARIES})

