cmake_minimum_required(VERSION 3.16)

project(MCPPPP
	DESCRIPTION "A resource pack converter from Optifine/MCPatcher formats to other formats"
	LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

option(MCPPPP_CLI "Build CLI version of MCPPPP" TRUE)
option(MCPPPP_GUI "Build GUI version of MCPPPP" TRUE)
option(MCPPPP_JNI "Build JNI libraries for MCPPPP mod" FALSE)

include_directories("include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(lodepng STATIC "src/lodepng.cpp" "include/lodepng.h")
set_property(TARGET lodepng PROPERTY POSITION_INDEPENDENT_CODE TRUE)

if (MCPPPP_CLI)
	add_executable(MCPPPP-cli "src/Source.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp")
	target_link_libraries(MCPPPP-cli lodepng)
endif ()

if (MCPPPP_GUI)
	set(FLTK_BUILD_TEST FALSE CACHE BOOL "Build test/demo programs" FORCE) # Don't tmake fltk build tests
	set(FLTK_BUILD_FLUID FALSE CACHE BOOL "Build fluid ui designer" FORCE) # Don't build fluid (I had to add this option myself)
	set(OPTION_BUILD_HTML_DOCUMENTATION FALSE CACHE BOOL "build html docs" FORCE) # Don't build fltk docs
	add_subdirectory("lib/fltk/fltk-1.3.6")
	add_executable(MCPPPP-gui "src/Source.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp" "src/fl_impl.cpp" "src/gui.cpp" "src/mcpppp.cxx")
	target_compile_definitions(MCPPPP-gui PUBLIC "GUI")
	if (APPLE)
		target_link_libraries(MCPPPP-gui lodepng fltk "-framework Cocoa")
	else ()
		target_link_libraries(MCPPPP-gui lodepng fltk)
	endif ()
endif ()

if (MCPPPP_JNI)
	find_package(JNI)
	if (JNI_FOUND)
		include_directories("${JNI_INCLUDE_DIRS}")
		add_library(mcpppp SHARED "src/jni.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp")
		set_property(TARGET mcpppp PROPERTY POSITION_INDEPENDENT_CODE TRUE)
		target_link_libraries(mcpppp lodepng)
	else ()
		message(SEND_ERROR "Jni not found")
	endif ()
endif ()
