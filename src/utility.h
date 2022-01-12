/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

constexpr auto VERSION = "0.5.8"; // MCPPPP version
constexpr int PACK_VER = 8; // pack.mcmeta pack format

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
	inline bool autodeletetemp = false, pauseonexit = true, dolog = true, dotimestamp = false, autoreconvert = false, fsbtransparent = true;
	inline int outputlevel = 3, loglevel = 1;
	inline std::ofstream logfile("mcpppp-log.txt");
	static std::string logfilename = "mcpppp-log.txt";

	inline std::set<std::string> paths = {};
	inline nlohmann::ordered_json config;

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

	[[noreturn]] void exit() noexcept;

	std::string lowercase(std::string str);

	auto localtime_rs(tm* tm, const time_t* time);

	std::string timestamp();

	std::string ununderscore(std::string str);

	void findreplace(std::string& source, const std::string& find, const std::string& replace);

	std::string oftoregex(std::string of);

	class outstream
	{
	private:
		friend outstream out(const short& level) noexcept;
		bool cout, file, err, first = false;
		short level;
		outstream(const bool& _first, const bool& _cout, const bool& _file, const bool& _err, const short& _level) noexcept
			: cout(_cout), file(_file), err(_err), first(_first), level(_level) {}
		// lol
		static char* dupstr(const std::string& s)
		{
			// add one for null character
			char* c = new char[s.size() + 1];
			strncpy(c, s.c_str(), s.size() + 1);
			return c;
		}
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
			if (cout)
			{
#ifdef GUI
				if (first)
				{
					sstream << (dotimestamp ? timestamp() : "");
				}
				sstream << value;
#else
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
#endif
			}
			if (file && logfile.good())
			{
				if (first)
				{
					logfile << timestamp();
				}
				logfile << value;
			}
			first = false;
			return *this;
		}

		outstream operator<<(const std::string& str);

		outstream operator<<(std::ostream& (*f)(std::ostream&));
	};

	outstream out(const short& level) noexcept;

	void copy(const std::filesystem::path& from, const std::filesystem::path& to);

	void checkpackver(const std::filesystem::path& path);

	bool findzipitem(const std::string& ziparchive, const std::string& itemtofind);

	bool findfolder(const std::string& path, const std::string& tofind, const bool& zip);

	void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa);

	void rezip(const std::string& folder, Zippy::ZipArchive& zipa);

	bool convert(const std::filesystem::path& path, const bool& dofsb = true, const bool& dovmt = true, const bool& docim = true);

	void setting(const std::string& option, const nlohmann::json& j);

	void readconfig();
}
