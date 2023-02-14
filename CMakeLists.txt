cmake_minimum_required(VERSION 3.15)

project(MCPPPP
	DESCRIPTION "A resource pack converter from Optifine/MCPatcher formats to other formats"
	LANGUAGES CXX
	VERSION 0.7.3
	HOMEPAGE_URL "http://github.com/supsm/MCPPPP"
)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if (result)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
	message(WARNING "IPO not supported. Error: ${output}")
endif()

option(MCPPPP_CLI "Build CLI version of MCPPPP" TRUE)
option(MCPPPP_GUI "Build GUI version of MCPPPP" TRUE)
option(MCPPPP_JNI "Build JNI libraries for MCPPPP mod" FALSE)
option(MCPPPP_WEB "Build web version of MCPPPP using emscripten" FALSE)
option(MCPPPP_BUILD_FLTK "Build fltk from source and ignore preinstalled fltk (if MCPPPP_GUI is true)" FALSE)
option(MCPPPP_STATIC_LINK "Static link standard library (if possible)" FALSE)

include_directories("include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(lodepng OBJECT "src/lodepng.cpp" "include/lodepng.h")
add_library(microtar OBJECT "src/microtar.cpp" "include/microtar.h")
add_library(fmt OBJECT "src/format.cc" "include/fmt/core.h" "include/fmt/format.h" "include/fmt/format-inl.h")

function(add_mp_flag target_name)
	if (MSVC)
		target_compile_options(${target_name} PRIVATE "/MP")
	endif ()
endfunction()

function(add_static_flag target_name)
	if (MCPPPP_STATIC_LINK AND (NOT MSVC) AND (NOT APPLE)) # Doesn't seem to work on mac, hopefully will be fixed
		target_link_libraries(${target_name} PRIVATE "-static")
	endif ()
endfunction()

if (MCPPPP_CLI)
	add_executable(MCPPPP-cli "src/Source.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp")
	target_link_libraries(MCPPPP-cli PRIVATE $<TARGET_OBJECTS:lodepng> $<TARGET_OBJECTS:microtar> $<TARGET_OBJECTS:fmt>)
	add_static_flag(MCPPPP-cli)
	add_mp_flag(MCPPPP-cli)
endif ()

if (MCPPPP_GUI)
	if (NOT MCPPPP_BUILD_FLTK)
		find_package(FLTK 1.3.6)
	endif ()
	if ((NOT FLTK_FOUND) OR MCPPPP_BUILD_FLTK)
		set(FLTK_LIBRARIES "") # clear this variable set by find_package
		set(FLTK_BUILD_TEST FALSE CACHE BOOL "Build test/demo programs" FORCE) # Don't tmake fltk build tests
		set(FLTK_BUILD_FLUID FALSE CACHE BOOL "Build fluid ui designer" FORCE) # Don't build fluid (I had to add this option myself)
		set(OPTION_BUILD_HTML_DOCUMENTATION FALSE CACHE BOOL "build html docs" FORCE) # Don't build fltk docs
		add_subdirectory("lib/fltk/fltk-1.3.6")
	endif ()
	add_executable(MCPPPP-gui "src/Source.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp" "src/fl_impl.cpp" "src/gui.cpp" "src/mcpppp.cxx")
	target_compile_definitions(MCPPPP-gui PUBLIC "GUI")
	target_link_libraries(MCPPPP-gui PRIVATE $<TARGET_OBJECTS:lodepng> $<TARGET_OBJECTS:microtar> $<TARGET_OBJECTS:fmt> fltk)
	# I guess this is needed?
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	target_link_libraries(MCPPPP-gui PRIVATE Threads::Threads)
	if (APPLE)
		target_link_libraries(MCPPPP-gui PRIVATE "-framework Cocoa")
	endif ()
	# No static link for gui (for now)
	add_mp_flag(MCPPPP-gui)
endif ()

if (MCPPPP_JNI)
	find_package(JNI)
	if (JNI_FOUND)
		include_directories("${JNI_INCLUDE_DIRS}")
		add_library(mcpppp SHARED "src/jni.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp")
		target_link_libraries(mcpppp PRIVATE $<TARGET_OBJECTS:lodepng> $<TARGET_OBJECTS:microtar> $<TARGET_OBJECTS:fmt>)
		add_mp_flag(mcpppp)
	else ()
		message(SEND_ERROR "Jni not found")
	endif ()
endif ()

if (MCPPPP_WEB)
	if (EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
		add_executable(MCPPPP-web "src/Source.cpp" "src/fsb.cpp" "src/vmt.cpp" "src/cim.cpp" "src/utility.cpp" "src/em_impl.cpp")
		target_link_libraries(MCPPPP-web PRIVATE $<TARGET_OBJECTS:lodepng> $<TARGET_OBJECTS:microtar> $<TARGET_OBJECTS:fmt>)
		target_compile_options(MCPPPP-web PRIVATE -sDISABLE_EXCEPTION_CATCHING=0 -pthread)
		target_link_libraries(MCPPPP-web PRIVATE "embind" "--shell-file ${CMAKE_SOURCE_DIR}/src/template.html" "-s EXIT_RUNTIME=0" "-s ALLOW_MEMORY_GROWTH" "-s DISABLE_EXCEPTION_CATCHING=0" "-pthread" "-s PTHREAD_POOL_SIZE=1")
	else ()
		message(SEND_ERROR "Web version must be compiled using emscripten (emcmake)")
	endif ()
endif ()
