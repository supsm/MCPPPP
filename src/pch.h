#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#ifdef __cpp_lib_concepts
#include <concepts>
#endif
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "microtar.h"
#include "argparse/argparse.hpp"

#include "lodepng.h"

#include "json.hpp"
#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif
#include "Zippy.hpp"

#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "xxhash.h"
