/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "pch.h"

#include "constants.h"
#include "convert.h"

#ifdef GUI
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include "mcpppp.h" // ui class
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/wasm_worker.h>
#include <emscripten/val.h>
#endif

#if defined(GUI) || defined(__EMSCRIPTEN__)
#define GUI_OUTPUT
#endif

namespace mcpppp
{
	enum class level_t { debug, detail, info, important, warning, error, system_info };
}

#define MCPPPP_ASSERT(condition)													\
if (!(condition))																	\
{																					\
	mcpppp::output<mcpppp::level_t::error>("Assertation failed: {}", #condition);	\
	abort();																		\
}

#define MCPPPP_CATCH_ALL()												\
catch (const nlohmann::json::exception& e)								\
{																		\
	output<level_t::error>("FATAL JSON ERROR:\n{}", e.what());			\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}																		\
catch (const Zippy::ZipLogicError& e)									\
{																		\
	output<level_t::error>("FATAL ZIP LOGIC ERROR:\n{}", e.what());		\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}																		\
catch (const Zippy::ZipRuntimeError& e)									\
{																		\
	output<level_t::error>("FATAL ZIP RUNTIME ERROR:\n{}", e.what());	\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}																		\
catch (const std::filesystem::filesystem_error& e)						\
{																		\
	output<level_t::error>("FATAL FILESYSTEM ERROR:\n{}", e.what());	\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}																		\
catch (const std::exception& e)											\
{																		\
	output<level_t::error>("FATAL ERROR:\n{}", e.what());				\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}																		\
catch (...)																\
{																		\
	output<level_t::error>("UNKNOWN FATAL ERROR");						\
	mcpppp::printpseudotrace();											\
	std::exit(-1);														\
}

namespace mcpppp
{
	enum class conversions { fsb, vmt, cim };

	enum class type_t { boolean, integer, string };


#ifdef GUI
	// fltk ui object
	inline std::unique_ptr<UI> ui;
#endif
#ifdef GUI_OUTPUT
	// stringstream containing current outputted line
	inline std::stringstream sstream;
#endif

	// number of arguments, copied from main
	inline int argc = -1;

	// paths to scan for resourcepacks
	inline std::set<std::filesystem::path> paths = {};
	// config settings/paths
	inline nlohmann::ordered_json config;

	class resourcepack_entry
	{
	public:
		bool selected; // whether path is selected for conversion, always true for cli
		bool force_reconvert; // force reconvert resource pack (i.e. if checkresult is reconverting, always convert)
		std::filesystem::directory_entry path_entry; // directory entry of resourcepack

#ifdef GUI
		std::unordered_map<conversions, checkresults> conv_statuses;

		std::unique_ptr<Fl_Check_Button> checkbox_widget;
		std::unique_ptr<Fl_Box> label_widget;
		std::unique_ptr<Fl_Menu_Button> right_click_widget;
		std::unique_ptr<char> menu_item_label_text;
		std::unique_ptr<int> current_numbuttons;
#endif

		resourcepack_entry(const bool selected_, const bool force_reconvert_,
			const std::filesystem::directory_entry& path_entry_) :
			selected(selected_), force_reconvert(force_reconvert_), path_entry(path_entry_) {}

		resourcepack_entry(const std::filesystem::directory_entry& path_entry_) :
			selected(true), force_reconvert(false), path_entry(path_entry_) {}

#ifdef GUI
		resourcepack_entry(const bool selected_, const bool force_reconvert_,
			const std::filesystem::directory_entry& path_entry_,
			const std::unordered_map<conversions, checkresults>& conv_statuses_,
			std::unique_ptr<Fl_Check_Button>&& checkbox_widget_,
			std::unique_ptr<Fl_Box>&& label_widget_,
			std::unique_ptr<Fl_Menu_Button>&& right_click_widget_,
			std::unique_ptr<char>&& menu_item_label_text_,
			std::unique_ptr<int>&& current_numbuttons_) :
			selected(selected_), force_reconvert(force_reconvert_), path_entry(path_entry_),
			conv_statuses(conv_statuses_),
			checkbox_widget(std::move(checkbox_widget_)),
			label_widget(std::move(label_widget_)),
			right_click_widget(std::move(right_click_widget_)),
			menu_item_label_text(std::move(menu_item_label_text_)),
			current_numbuttons(std::move(current_numbuttons_)) {}

		resourcepack_entry(const std::filesystem::directory_entry& path_entry_,
			const std::unordered_map<conversions, checkresults>& conv_statuses_,
			std::unique_ptr<Fl_Check_Button>&& checkbox_widget_,
			std::unique_ptr<Fl_Box>&& label_widget_,
			std::unique_ptr<Fl_Menu_Button>&& right_click_widget_,
			std::unique_ptr<char>&& menu_item_label_text_,
			std::unique_ptr<int>&& current_numbuttons_) :
			selected(true), force_reconvert(false), path_entry(path_entry_),
			conv_statuses(conv_statuses_),
			checkbox_widget(std::move(checkbox_widget_)),
			label_widget(std::move(label_widget_)),
			right_click_widget(std::move(right_click_widget_)),
			menu_item_label_text(std::move(menu_item_label_text_)),
			current_numbuttons(std::move(current_numbuttons_)) {}
#endif
	};

	// list of resourcepacks to convert
	inline std::vector<resourcepack_entry> entries = {};

	// hashes of converted resourcepacks
	inline nlohmann::json hashes;


	// info for each settting item
	class setting_item
	{
	public:
		// capitalized name of setting, since key is all lowercase
		std::string_view formatted_name;
		std::string_view description;

		// data type of setting (bool, int, string, etc)
		type_t type;
		// reference of setting variable to update
		std::variant<std::reference_wrapper<bool>, std::reference_wrapper<level_t>, std::reference_wrapper<std::string>> var;
		// default value of setting
		// json since it is compared with another json value in save_settings
		nlohmann::json default_val;
		// minimum and maximum value for integer type settings
		int min = 0, max = 0;

	private:
		template<typename T, typename container>
		static constexpr bool has_type = false;

	public:
		template<typename T> requires has_type<std::reference_wrapper<T>, decltype(var)>
		T& get() const noexcept
		{
			return std::get<std::reference_wrapper<T>>(var).get();
		}
	};

	// gcc errors if I put this inside the class
	template<typename T, template<typename...> typename container, typename... Args>
	constexpr bool setting_item::has_type<T, container<Args...>> =
		std::disjunction_v<std::is_same<T, Args>...>;

	// settings
	inline bool autodeletetemp = false; // automatically delete mcpppp-temp if found
	inline bool pauseonexit = true; // pause and ask for user input upon conversion finishing
	inline bool dolog = true; // log to file
	inline bool dotimestamp = false; // add timestamp to regular output
	inline bool autoreconvert = false; // automatically reconvert when resourcepacks are changed
	inline level_t outputlevel = level_t::important; // amount of info to output
	inline level_t loglevel = level_t::debug; // amount of info to output to log
	inline std::ofstream logfile("mcpppp-log.txt"); // log file
	static std::string logfilename = "mcpppp-log.txt"; // name of log file

	// info about each setting, key should be all lowercase
	const std::unordered_map<std::string, setting_item> settings =
	{
		{"pauseonexit", {"pauseOnExit", "Wait for enter key to be pressed once execution has been finished",
			type_t::boolean, std::ref(pauseonexit), pauseonexit}},
		{"log", {"log", "A log file where logs will be stored. \"\" disables logging",
			type_t::string, logfilename, std::ref(logfilename)}},
		{"timestamp", {"timestamp", "Add timestamp to regular output (Logs will always be timestamped)",
			type_t::boolean, std::ref(dotimestamp), dotimestamp}},
		{"autodeletetemp", {"autoDeleteTemp", "Automatically delete mcpppp-temp folder on startup",
			type_t::boolean, std::ref(autodeletetemp), autodeletetemp}},
		{"outputlevel", {"outputLevel", "How much info should be outputted. 0 is most info (debug), 5 is least info (errors only)",
			type_t::integer, std::ref(outputlevel), static_cast<int>(outputlevel), 0, 5}},
		{"loglevel", {"logLevel", "How much info should be outputted to log; 0 is most, 5 is least.\nHas no effect if log is not set",
			type_t::integer, std::ref(loglevel), static_cast<int>(loglevel), 0, 5}},
		{"autoreconvert", {"autoReconvert", "Automatically reconvert changed resourcepacks instead of skipping. Only checks packs that have previously been converted",
			type_t::boolean, std::ref(autoreconvert), autoreconvert}}
	};


#ifdef GUI_OUTPUT
	// vector of things already outputted, to be used when outputlevel is changed
	// pair of {level, text}
	inline std::vector<std::pair<short, std::string>> outputted;

	// mutex for accessing outputted vector
	inline std::mutex output_mutex;
#endif
#ifdef GUI
	// pack warning messages which are all outputted after conversion finishes
	inline std::vector<std::string> alerts;

	inline std::atomic_bool pause_conversion;
#endif

#ifdef __cpp_lib_source_location
	// recently visited locations, may be useful for debugging
	// replace with <stacktrace> in C++23
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
		try
		{
			pseudotrace.push_back(item);
			if (pseudotrace.size() > maxtracesize)
			{
				pseudotrace.pop_front();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			if (dolog && logfile.good())
			{
				logfile << "Error: " << e.what() << std::endl;
			}
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

	// make string lowercase
	// @param str  string to convert to lowercase (passed by value)
	// @return lowercase version of `str`
	inline std::u8string lowercase(std::u8string str)
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
	void findreplace(std::string& source, const std::string_view& find, const std::string_view& replace);

	// find and replace (with u8strings)
	// @param source  string to modify
	// @param find  string to find in `source`
	// @param replace  string to replace `find` with
	void findreplace(std::u8string& source, const std::u8string_view& find, const std::u8string_view& replace);

	// convert std::u8string to std::string
	inline std::string c8tomb(const std::u8string_view& s)
	{
		return std::string(s.begin(), s.end());
	}

	// convert char8_t[] (u8string) to char[] (string)
	inline const char* c8tomb(const char8_t* s)
	{
		return reinterpret_cast<const char*>(s);
	}

	// convert std::string to std::u8string
	inline std::u8string mbtoc8(const std::string_view& s)
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
		std::string getfilenamehash(const std::filesystem::path& path, bool zip);

		// parse contents of .properties into map
		// @param data  contents of properties file
		// @return vector of key value pairs
		std::unordered_map<std::string, std::string> parse_properties(const std::string_view& data);

		// parse a range (can contain negative)
		// @param s  string to parse
		// @return pair of height values, or { INT_MIN, INT_MIN } on failure
		std::pair<int, int> parse_range(const std::string_view& s);
	}

	// lol
	// create a char* copy of a string
	// @param s  string to copy
	// @return char pointer, copy of `s`
	inline char* dupstr(const std::string_view& s)
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

		friend void printpseudotrace(unsigned int numlines) noexcept;

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
			try
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
					std::scoped_lock lock(output_mutex);
					// std::to_underlying in C++23
					outputted.emplace_back(static_cast<int>(level), line); // we don't need the modifier stuffs since we can add them later on
				}
				sstream.str(std::string());
				sstream.clear();
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error: " << e.what() << std::endl;
				if (file && logfile.good())
				{
					logfile << "Error: " << e.what() << std::endl;
				}
			}
		}
#elif defined(__EMSCRIPTEN__)
		// @param str  line to output, as a char pointer
		// @param color_name  name of color, as a char pointer
		static void update_output_main_thread(int str, int color_name) noexcept
		{
			EM_ASM({
				output(UTF8ToString($0), UTF8ToString($1));
				}, str, color_name);
			delete reinterpret_cast<char*>(str);
		}

		void updateoutput() const noexcept
		{
			std::string line;
			while (std::getline(sstream, line))
			{
				if (cout)
				{
					MAIN_THREAD_EM_ASM({ output(UTF8ToString($0), UTF8ToString($1)); },
						line.c_str(), colors.at(static_cast<size_t>(level)).data());
				}
				std::scoped_lock lock(output_mutex);
				outputted.emplace_back(static_cast<short>(level), line);
			}
			sstream.str({});
			sstream.clear();
		}
#endif

	public:
		~outstream() noexcept
		{
#ifdef GUI_OUTPUT
			if (argc < 2)
			{
				updateoutput();
			}
#else
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
#endif
			if (file)
			{
#ifdef __EMSCRIPTEN__
				std::cout << std::endl;
#else
				if (logfile.good())
				{
					logfile << std::endl;
				}
#endif
			}
		}

#ifdef GUI
		// colors to use when outputting
		// couldn't find a good pre-defined color for warning
		// this is public so it can be used for redrawing when output level is changed
		static constexpr std::array<Fl_Color, 7> colors = { FL_DARK2, FL_DARK3, FL_FOREGROUND_COLOR, FL_DARK_GREEN, 92, FL_RED, FL_DARK_MAGENTA };
#elif defined(__EMSCRIPTEN__)
		// colors to use when outputting (html names)
		static constexpr std::array<std::string_view, 7> colors = { "LightGray", "Gray", "Black", "Green", "DarkOrange", "Red", "Purple" };
#endif

		// template functions must be defined in header
		template<outputtable T>
		const outstream& operator<<(const T& value) const noexcept
		{
#ifdef GUI_OUTPUT
			// if there are no command line arguments, ignore level print to gui
			// otherwise, print to command line like cli
			if (argc < 2)
			{
				sstream << value;
			}
#else
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
#endif
			if (file)
			{
#ifdef __EMSCRIPTEN__
				std::cout << value;
#else
				if (logfile.good())
				{
					logfile << value;
				}
#endif
			}
			return *this;
		}

		const outstream& operator<<(const std::string_view& str) const noexcept;

		const outstream& operator<<(std::ostream& (*f)(std::ostream&)) const noexcept;
	};

	// prints all lines from pseudotrace
	// @param numlines  max number of lines to output, 0 = all lines
	inline void printpseudotrace(const unsigned int numlines = 0) noexcept
	{
#ifdef __cpp_lib_source_location
		try
		{
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
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
#ifndef __EMSCRIPTEN__
			if (dolog && logfile.good())
			{
				logfile << "Error: " << e.what() << std::endl;
			}
#endif
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

		if (level >= loglevel && dolog)
		{
#ifdef __EMSCRIPTEN__
			std::cout << timestamp() << ' ';
#else
			if (logfile.good())
			{
				logfile << timestamp() << ' ';
			}
#endif
		}
#ifdef GUI_OUTPUT
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
			outstream out(level >= outputlevel, dolog && level >= loglevel, level == level_t::error, level);
			out << fmt::format(fmt::runtime(fmt.fmt), args...); // i don't know why `fmt.fmt` isn't compile-time enough
		}
#ifdef __cpp_lib_source_location
		{
			outstream debug_out(level_t::debug >= outputlevel, dolog && level_t::debug >= loglevel, false, level_t::debug);
			debug_out << fmt::format(location_format, fmt.location.file_name(), fmt.location.function_name(), fmt.location.line(), fmt.location.column());
		}
#endif

		if (level == level_t::error)
		{
			printpseudotrace(4);
		}
	}

	// do things when a checkpoint is hit
	// e.g. check for pause, return control to webpage, update progress bar
	inline void do_checkpoint_stuff()
	{
#ifdef GUI
		while (pause_conversion)
		{
			std::this_thread::yield();
		}
#endif
	}

#ifdef __cpp_lib_source_location
	// only log checkpoint, do not perform other actions
	// should not block
	inline void checkpoint_only(std::source_location location = std::source_location::current())
	{
		addtraceitem(location);
		output<level_t::debug>(location_format, location.file_name(), location.function_name(), location.line(), location.column());
	}

	// creates a checkpoint
	// see do_checkpoint_stuff
	inline void checkpoint(std::source_location location = std::source_location::current())
	{
		checkpoint_only(location);
		do_checkpoint_stuff();
	}
#else
	inline void checkpoint_only() {}

	// creates a checkpoint
	// see do_checkpoint_stuff
	inline void checkpoint()
	{
		checkpoint_only();
		do_checkpoint_stuff();
	}
#endif

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
	bool findfolder(const std::filesystem::path& path, const std::u8string_view& tofind, bool zip) noexcept;


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

		checkpoint();
		return gethex(rawhash);
	}

	// get statuses of conversions
	// @param path  path of resource pack to check
	// @param dofsb  whether to check fsb (optional, default true)
	// @param dovmt  whether to check vmt (optional, default true)
	// @param docim  whether to check cim (optional, default true)
	// @return map of each conversion to a checkresults
	std::unordered_map<conversions, checkresults> getconvstatus(const std::filesystem::path& path, bool dofsb, bool dovmt, bool docim) noexcept;

	// convert a single resource pack
	// @param path  path of resource pack to convert
	// @param dofsb  whether to convert fsb (optional, default true)
	// @param dovmt  whether to convert vmt (optional, default true)
	// @param docim  whether to convert cim (optional, default true)
	// @param force_reconvert  ignore reconvert checking and always reconvert. may lose data (optional, default false)
	// @return whether conversion succeeds
	bool convert(const std::filesystem::path& path, bool dofsb = true, bool dovmt = true, bool docim = true, bool force_reconvert = false);

	// read hashes from mcpppp-hashes.json to `hashes`
	void gethashes();

	// output hashes to mcpppp-hashes.json
	void savehashes();

	// parse `config` json and set necessary variables
	void readconfig();

	// parse command-line arguments
	void parseargs(int argc, const char* argv[]);
}
