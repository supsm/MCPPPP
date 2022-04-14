/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef _WIN32
#define NOMINMAX
#endif

#include <atomic>
#ifdef __cpp_lib_concepts
#include <concepts>
#endif
#include <fmt/core.h>
#include <fstream>
#include <iomanip>
#include <list>
#include <mutex>
#include <set>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif
#include <sstream>
#include <thread>
#include <unordered_map>
#include <variant>

#include "constants.h"

#ifdef GUI
#include <FL/Fl.H>
#include "mcpppp.h" // ui class
#endif

#include "json.hpp"
#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif
#include "Zippy.hpp"

#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "xxhash.h"

namespace mcpppp
{
	enum class level_t { debug, detail, info, important, warning, error, system_info };
}

#define MCPPPP_ASSERT(condition)                                                                  \
if (!(condition))                                                                                 \
{                                                                                                 \
	mcpppp::output<mcpppp::level_t::error>("Assertation failed: {}", #condition); \
	abort();                                                                                      \
}

namespace mcpppp
{
#ifdef GUI
	// fltk ui object
	inline std::unique_ptr<UI> ui;
	// stringstream containing current outputted line
	inline std::stringstream sstream;
#endif
	// number of arguments, copied from main
	inline int argc = -1;

	// settings
	inline bool autodeletetemp = false; // automatically delete mcpppp-temp if found
	inline bool pauseonexit = true; // pause and ask for user input upon conversion finishing
	inline bool dolog = true; // log to file
	inline bool dotimestamp = false; // add timestamp to regular output
	inline bool autoreconvert = false; // automatically reconvert when resourcepacks are changed
	inline bool fsbtransparent = true; // make fsb conversion transparent, similar to optifine
	inline level_t outputlevel = level_t::important; // amount of info to output
	inline level_t loglevel = level_t::debug; // amount of info to output to log
	inline std::ofstream logfile("mcpppp-log.txt"); // log file
	static std::string logfilename = "mcpppp-log.txt"; // name of log file

	// paths to scan for resourcepacks
	inline std::set<std::filesystem::path> paths = {};
	// config settings/paths
	inline nlohmann::ordered_json config;
	// list of resourcepacks to convert
	inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};

	// hashes of converted resourcepacks
	inline nlohmann::json hashes;

	enum class type { boolean, integer, string };

	// info for each settting item
	struct setting_item
	{
		// data type of setting (bool, int, string, etc
		type setting_type;
		// reference of setting variable to update
		std::variant<std::reference_wrapper<bool>, std::reference_wrapper<level_t>, std::reference_wrapper<std::string>> var;
		// default value of setting
		// json since it is compared with another json value in save_settings
		nlohmann::json default_val;
		// minimum and maximum value for integer type settings
		int min = 0, max = 0;
	};

	// info about each setting
	const std::unordered_map<std::string, setting_item> settings =
	{
		{"pauseonexit", {type::boolean, std::ref(pauseonexit), pauseonexit}},
		{"log", {type::string, logfilename, std::ref(logfilename)}},
		{"timestamp", {type::boolean, std::ref(dotimestamp), dotimestamp}},
		{"autodeletetemp", {type::boolean, std::ref(autodeletetemp), autodeletetemp}},
		{"outputlevel", {type::integer, std::ref(outputlevel), static_cast<int>(outputlevel), 0, 5}},
		{"loglevel", {type::integer, std::ref(loglevel), static_cast<int>(loglevel), 0, 5}},
		{"autoreconvert", {type::boolean, std::ref(autoreconvert), autoreconvert}},
		{"fsbtransparent", {type::boolean, std::ref(fsbtransparent), fsbtransparent}}
	};

#ifdef GUI
	// vector of things already outputted, to be used when outputlevel is changed
	// pair of {level, text}
	inline std::vector<std::pair<short, std::string>> outputted;

	// mutex for accessing outputted vector
	inline std::mutex output_mutex;
#endif

#ifdef __cpp_lib_source_location
	// recently visited locations, may be useful for debugging
	inline std::list<std::source_location> pseudotrace;
#endif

	// don't output probably since output is being redrawn
	inline std::atomic_bool waitdontoutput = false;

	// exit by pausing (if necessary) and exiting with code 0
	[[noreturn]] inline void exit()
	{
#ifndef GUI
		if (pauseonexit)
		{
			std::string str;
			std::cout << "Press enter to continue . . .";
			getline(std::cin, str);
		}
#endif
		std::exit(0);
	}

#ifdef __cpp_lib_source_location
	// add item to pseudotrace, removing excess if necessary
	// @param item  item to add
	inline void addtraceitem(const std::source_location& item)
	{
		pseudotrace.push_back(item);
		if (pseudotrace.size() > maxtracesize)
		{
			pseudotrace.pop_front();
		}
	}
#endif

	// make string lowercase
	// @param str  string to convert to lowercase (passed by value)
	// @return lowercase version of `str`
	inline std::string lowercase(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](const char& c) -> char
			{
				return std::tolower(c);
			});
		return str;
	}

	// convert bool to "true" or "false"
	// @param b  bool to convert to text
	// @return "true" if b is true and "false" for false
	constexpr std::string_view boolalpha(const bool b)
	{
		return (b ? "true" : "false");
	}

	// get timestamp of format [hh:mm:ss]
	// @return current timestamp
	std::string timestamp() noexcept;

	// find and replace a string with another string
	// @param source  string to modify
	// @param find  string to find in `source`
	// @param replace  string to replace find with
	void findreplace(std::string& source, const std::string& find, const std::string& replace);

	// find and replace (with u8strings)
	// @param source  string to modify
	// @param find  string to find in `source`
	// @param replace  string to replace `find` with
	void findreplace(std::u8string& source, const std::u8string& find, const std::u8string& replace);

	// convert std::u8string to std::string
	inline std::string c8tomb(const std::u8string& s)
	{
		return std::string(s.begin(), s.end());
	}

	// convert char8_t[] (u8string) to char[] (string)
	inline const char* c8tomb(const char8_t* s)
	{
		return reinterpret_cast<const char*>(s);
	}

	// convert std::string to std::u8string
	inline std::u8string mbtoc8(const std::string& s)
	{
		return std::u8string(s.begin(), s.end());
	}

	// convert char[] (string) to char8_t[] (u8string)
	inline const char8_t* mbtoc8(const char* s)
	{
		return reinterpret_cast<const char8_t*>(s);
	}

	// utility functions exclusively for conversion
	namespace conv
	{
		// remove underscore '_' character from string
		// @param str  string to remove underscores from (passed by value)
		// @return `str` with underscores removed
		std::string ununderscore(std::string str);

		// convert optifine regex-like format to java regex
		// @param of  string of optifine format
		// @return java regex string
		std::string oftoregex(std::string of);

		// get hash of resourcepack filename
		// @param path  path to resource pack
		// @param zip  whether resource pack is zip
		// @return hex representation of hash
		std::string getfilenamehash(const std::filesystem::path& path, const bool zip);

		// parse contents of .properties into map
		// @param data  contents of properties file
		// @return vector of key value pairs
		std::unordered_map<std::string, std::string> parse_properties(const std::string& data);
	}

	// lol
	// create a char* copy of a string
	// @param s  string to copy
	// @return char pointer, copy of `s`
	inline char* dupstr(const std::string& s)
	{
		// add one for null character
		char* c = new char[s.size() + 1]{};
		std::copy_n(s.begin(), s.size(), c);
		return c;
	}

	// I love these new concept things
	template<typename T>
	concept outputtable = requires(T a)
	{
		std::ostringstream() << a;
	};

	// hacky thing so the source location can be implicitly set
	struct format_location
	{
		std::string_view fmt;
#ifdef __cpp_lib_source_location
		std::source_location location;
#endif

		template <
#ifdef __cpp_lib_concepts
			std::convertible_to<std::string_view>
#else
			typename
#endif
			T>
		consteval format_location(T fmt
#ifdef __cpp_lib_source_location
			, std::source_location location = std::source_location::current()
#endif
		) noexcept : fmt(fmt)
#ifdef __cpp_lib_source_location
			, location(location)
#endif
		{}
	};

	template<typename T>
	concept formattable = requires(T a)
	{
		fmt::make_format_args(a);
	};

	// object to conditionally output to log file and regular output
	class outstream
	{
	private:
		template <level_t level, formattable... Args>
		friend void output(const format_location& fmt, Args&&... args) noexcept;

		friend void printpseudotrace(const unsigned int numlines) noexcept;

		bool cout; // whether to output to regular output
		bool file; // whether to output to log file
		bool err; // whether to output to stderr if `cout` is true
		level_t level; // current output level, used to determine color of output in gui
		outstream(const bool _cout, const bool _file, const bool _err, const level_t& _level) noexcept
			: cout(_cout), file(_file), err(_err), level(_level) {}
#ifdef GUI
		// output to gui window
		// @param v  char* to output, cast to void*
		static void print(void* v) noexcept
		{
			ui->output->add(static_cast<char*>(v));
			ui->output->bottomline(ui->output->size()); // automatically scroll to the bottom
			delete[] static_cast<char*>(v);
		}

		// process and output to gui window
		void updateoutput() const noexcept
		{
			if (sstream.str().empty())
			{
				sstream.str(" "); // output line even if empty
			}
			while (waitdontoutput)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			std::string line;
			sstream.put(' ');
			while (std::getline(sstream, line))
			{
				if (line.empty())
				{
					line = " "; // fltk won't print empty strings
				}
				// add color and output line
				if (cout)
				{
					Fl::awake(print, dupstr(fmt::format("@S14@C{}@.{}", colors.at(static_cast<size_t>(level)), line)));
				}
				output_mutex.lock();
				outputted.emplace_back(static_cast<int>(level), line); // we don't need the modifier stuffs since we can add them later on
				output_mutex.unlock();
			}
			sstream.str(std::string());
			sstream.clear();
		}
#endif
	public:
		~outstream()
		{
			if (file && logfile.good())
			{
				logfile << std::endl;
			}
#ifdef GUI
			if (argc < 2)
			{
				updateoutput();
				return;
			}
#endif
			if (cout)
			{
				if (err)
				{
					std::cerr << std::endl;
				}
				else
				{
					std::cout << std::endl;
				}
			}
		}

#ifdef GUI
		// colors to use when outputting
		// couldn't find a good pre-defined color for warning
		// this is public so it can be used for redrawing when output level is changed
		static constexpr std::array<Fl_Color, 7> colors = { FL_DARK2, FL_DARK3, FL_FOREGROUND_COLOR, FL_DARK_GREEN, 92, FL_RED, FL_DARK_MAGENTA };
#endif

		// template functions must be defined in header
		template<outputtable T>
		const outstream& operator<<(const T& value) const noexcept
		{
			if (file && logfile.good())
			{
				logfile << value;
			}
#ifdef GUI
			// if there are no command line arguments, ignore level print to gui
			// otherwise, print to command line like cli
			if (argc < 2)
			{
				sstream << value;
				return *this;
			}
#endif
			if (cout)
			{
				if (err)
				{
					std::cerr << value;
				}
				else
				{
					std::cout << value;
				}
			}
			return *this;
		}

		const outstream& operator<<(const std::string& str) const noexcept;

		const outstream& operator<<(std::ostream& (*f)(std::ostream&)) const noexcept;
	};

	// prints all lines from pseudotrace
	// @param numlines  max number of lines to output, 0 = all lines
	inline void printpseudotrace(const unsigned int numlines = 0) noexcept
	{
#ifdef __cpp_lib_source_location
		unsigned int num = 0;
		for (const auto& location : pseudotrace)
		{
			if (numlines != 0 && num > numlines)
			{
				break;
			}
			outstream out(true, true, true, level_t::error);
			out << '\t' << fmt::format(location_format, location.file_name(), location.function_name(), location.line(), location.column());
			num++;
		}
#endif
	}

	// wrapper of outstream using fmt::format, with location information
	// @param fmt  format string, needs to be convertible to std::string_view, needs to be constant expression
	// @param args  arguments used in fmt
	template <level_t level, formattable... Args>
	inline void output(const format_location& fmt, Args&&... args) noexcept
	{
#ifdef __cpp_lib_source_location
		addtraceitem(fmt.location);
#endif

		if (level >= loglevel && logfile.good())
		{
			logfile << timestamp() << ' ';
		}
#ifdef GUI
		if (argc < 2)
		{
			sstream << (dotimestamp ? timestamp() + ' ' : "");
		}
		else if
#else
		if
#endif
			(level >= outputlevel)
		{
			if (level == level_t::error)
			{
				std::cerr << (dotimestamp ? timestamp() + ' ' : "");
			}
			else
			{
				std::cout << (dotimestamp ? timestamp() + ' ' : "");
			}
		}
		{
			outstream out(level >= outputlevel, level >= loglevel, level == level_t::error, level);
			out << fmt::format(fmt::runtime(fmt.fmt), args...); // i don't know why `fmt.fmt` isn't compile-time enough
		}
#ifdef __cpp_lib_source_location
		{
			outstream debug_out(level_t::debug >= outputlevel, level_t::debug >= loglevel, false, level_t::debug);
			debug_out << fmt::format(location_format, fmt.location.file_name(), fmt.location.function_name(), fmt.location.line(), fmt.location.column());
		}
#endif

		if (level == level_t::error)
		{
			printpseudotrace(4);
		}
	}

	// copy file/folder to another location
	// @param from  file/folder to copy
	// @param to  file/folder to copy to
	// @return whether the file was copied successfully
	bool copy(const std::filesystem::path& from, const std::filesystem::path& to) noexcept;

	// find folder in resource pack
	// @param path  path of resource pack to search in
	// @param tofind  path of folder to find
	// @param zip  whether the resource pack is a .zip file
	// @return whether the folder was found
	bool findfolder(const std::filesystem::path& path, const std::u8string& tofind, const bool zip);


	// hash any arbitrary block of data into hex  
	// @tparam hash_size  size of hash, may be padded to fit into hex. recommended sizes 32, 64, 128 (max 128, default 64)
	// @param data  data to hash
	// @param size  size of data
	// @param seed  seed to use when hashing (optional, default 0). Will be cast to XXH32_hash_t if hash_size <=32
	// @return hex representation of hash
	template<short hash_size = 64>
	inline std::string hash(const void* data, const size_t size, const XXH64_hash_t& seed = 0)
	{
		static_assert(hash_size <= 128, "Hash size must be <=128");

		const auto gethex = [](const std::variant<std::monostate, XXH32_hash_t, XXH64_hash_t, XXH128_hash_t>& rawhash) -> std::string
		{
			std::ostringstream ss;
			ss << std::setfill('0') << std::hex;

			if (rawhash.index() == 0) // type is monostate
			{
				output<level_t::error>("Error: Hash failed somehow???");
				return std::string();
			}

			if constexpr (hash_size <= 32)
			{
				ss << std::setw(hash_size / 4) << std::get<XXH32_hash_t>(rawhash);
			}
			else if constexpr (hash_size <= 64) // 32 < size <= 64
			{
				ss << std::setw(hash_size / 4) << std::get<XXH64_hash_t>(rawhash);
			}
			else // 64 < size <= 128
			{
				const auto rawhashval = std::get<XXH128_hash_t>(rawhash);
				ss << std::setw(16) << rawhashval.high64 <<
					std::setw((hash_size - 64) / 4) << rawhashval.low64;
			}

			return ss.str();
		};

		std::variant<std::monostate, XXH32_hash_t, XXH64_hash_t, XXH128_hash_t> rawhash;

		if constexpr (hash_size <= 32)
		{
			rawhash = XXH32(data, size, static_cast<XXH32_hash_t>(seed));
		}
		else if constexpr (hash_size <= 64) // 32 < size <= 64
		{
			rawhash = XXH3_64bits_withSeed(data, size, seed);
		}
		else // 64 < size <= 128
		{
			rawhash = XXH3_128bits_withSeed(data, size, seed);
		}

		return gethex(rawhash);
	}

	// convert a single resource pack
	// @param path  path of resource pack to convert
	// @param dofsb  whether to convert fsb (optional, default true)
	// @param dovmt  whether to convert vmt (optional, default true)
	// @param docim  whether to convert cim (optional, default true)
	// @return whether conversion succeeds
	bool convert(const std::filesystem::path& path, const bool dofsb = true, const bool dovmt = true, const bool docim = true);

	// read hashes from mcpppp-hashes.json to `hashes`
	void gethashes();

	// output hashes to mcpppp-hashes.json
	void savehashes();

	// parse `config` json and set necessary variables
	void readconfig();

	// parse command-line arguments
	void parseargs(int argc, const char* argv[]);
}
