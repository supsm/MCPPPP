/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef _WIN32
#define NOMINMAX
#endif

#include <atomic>
#include <fstream>
#include <set>
#include <sstream>
#include <variant>

#ifdef GUI
#include <FL/Fl.H>
#include "mcpppp.h" // ui class
#endif

#include "json.hpp"
#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif
#include "Zippy.hpp"

namespace mcpppp
{
#ifdef GUI
	inline std::unique_ptr<UI> ui;
	inline std::stringstream sstream;
#endif
	inline int argc = -1;
	inline bool autodeletetemp = false, pauseonexit = true, dolog = true, dotimestamp = false, autoreconvert = false, fsbtransparent = true;
	inline int outputlevel = 3, loglevel = 1;
	inline std::ofstream logfile("mcpppp-log.txt");
	static std::string logfilename = "mcpppp-log.txt";

	inline std::set<std::filesystem::path> paths = {};
	inline nlohmann::ordered_json config;
	inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};

	inline nlohmann::json hashes;

	enum class type { boolean, integer, string };

	struct setting_item
	{
		type setting_type;
		std::variant<std::reference_wrapper<bool>, std::reference_wrapper<int>, std::reference_wrapper<std::string>> var;
		// json since it is compared with another json value in save_settings
		nlohmann::json default_val;
		int min = 0, max = 0;
	};

	const std::unordered_map<std::string, setting_item> settings =
	{
		{"pauseonexit", {type::boolean, std::ref(pauseonexit), pauseonexit}},
		{"log", {type::string, logfilename, std::ref(logfilename)}},
		{"timestamp", {type::boolean, std::ref(dotimestamp), dotimestamp}},
		{"autodeletetemp", {type::boolean, std::ref(autodeletetemp), autodeletetemp}},
		{"outputlevel", {type::integer, std::ref(outputlevel), outputlevel, 1, 5}},
		{"loglevel", {type::integer, std::ref(loglevel), loglevel, 1, 5}},
		{"autoreconvert", {type::boolean, std::ref(autoreconvert), autoreconvert}},
		{"fsbtransparent", {type::boolean, std::ref(fsbtransparent), fsbtransparent}}
	};

#ifdef GUI
	// vector of things already outputted, to be used when outputlevel is changed
	// level, text
	inline std::vector<std::pair<short, std::string>> outputted;
#endif

	inline std::atomic_bool waitdontoutput = false; // don't output probably since output is being redrawn

	[[noreturn]] void exit();

	std::string lowercase(std::string str);

	std::string timestamp();

	std::string ununderscore(std::string str);

	void findreplace(std::string& source, const std::string& find, const std::string& replace);

	void findreplace(std::u8string& source, const std::u8string& find, const std::u8string& replace);

	std::string c8tomb(const std::u8string& s);

	const char* c8tomb(const char8_t* s);

	std::u8string mbtoc8(const std::string& s);

	const char8_t* mbtoc8(const char* s);

	std::string oftoregex(std::string of);

	// lol
	inline char* dupstr(const std::string& s)
	{
		// add one for null character
		char* c = new char[s.size() + 1]{};
		std::copy_n(s.begin(), s.size(), c);
		return c;
	}

	/*// I love these new concept things
	template<typename T>
	concept outputtable = requires(T a)
	{
		std::stringstream() << a;
	};*/

	class outstream
	{
	private:
		friend outstream out(const short& level) noexcept;
		bool cout, file, err, first = false;
		short level;
		outstream(const bool& _first, const bool& _cout, const bool& _file, const bool& _err, const short& _level) noexcept
			: cout(_cout), file(_file), err(_err), first(_first), level(_level) {}
#ifdef GUI
		static void print(void* v);
#endif
	public:
#ifdef GUI
		// couldn't find a good pre-defined color for warning
		// this is public so it can be used for redrawing when output level is changed
		static constexpr std::array<Fl_Color, 6> colors = { FL_DARK3, FL_FOREGROUND_COLOR, FL_DARK_GREEN, 92, FL_RED, FL_DARK_MAGENTA };
#endif
		// template functions must be defined in header
		template<typename T>
		outstream operator<<(const T& value)
		{
			if (file && logfile.good())
			{
				if (first)
				{
					logfile << timestamp();
				}
				logfile << value;
			}
#ifdef GUI
			// if there are no command line arguments, ignore level print to gui
			// otherwise, print to command line like cli
			if (argc < 2)
			{
				if (first)
				{
					sstream << (dotimestamp ? timestamp() : "");
				}
				sstream << value;
				first = false;
				return *this;
			}
#endif
			if (cout)
			{
				if (first)
				{
					if (err)
					{
						std::cerr << (dotimestamp ? timestamp() : "");
					}
					else
					{
						std::cout << (dotimestamp ? timestamp() : "");
					}
				}
				if (err)
				{
					std::cerr << value;
				}
				else
				{
					std::cout << value;
				}
			}
			first = false;
			return *this;
		}

		outstream operator<<(const std::string& str);

		outstream operator<<(std::ostream& (*f)(std::ostream&));
	};

	outstream out(const short& level) noexcept;

	void copy(const std::filesystem::path& from, const std::filesystem::path& to);

	bool findfolder(const std::u8string& path, const std::u8string& tofind, const bool& zip);

	bool convert(const std::filesystem::path& path, const bool& dofsb = true, const bool& dovmt = true, const bool& docim = true);

	void gethashes();

	void savehashes();

	void setting(const std::string& option, const nlohmann::json& j);

	void readconfig();

	void parseargs(int argc, const char* argv[]);
}
