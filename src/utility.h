/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

constexpr auto VERSION = "0.5.8"; // MCPPPP version
constexpr int PACK_VER = 8; // pack.mcmeta pack format

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json.hpp"

#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif

#include "Zippy.hpp"

#ifdef GUI
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Radio_Button.H>
#include <FL/fl_ask.H>
#include "mcpppp.h" // ui class

namespace mcpppp
{
	static std::stringstream sstream;
	static Fl_Text_Buffer textbuffer;
	inline std::unique_ptr<UI> ui;
}
#endif

namespace mcpppp
{
	inline bool autodeletetemp = false, pauseonexit = true, dolog = true, dotimestamp = false, autoreconvert = false, fsbtransparent = true;
	inline int outputlevel = 3, loglevel = 1;
	inline std::ofstream logfile("mcpppp-log.txt");
	static std::string logfilename = "mcpppp-log.txt";

	inline std::set<std::string> paths = {};
	inline nlohmann::ordered_json config;

	std::atomic_bool wait_close; // wait for dialog to close

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
	static std::vector<std::pair<short, std::string>> outputted;
#endif


	[[noreturn]] inline void exit() noexcept
	{
#ifndef GUI
		if (pauseonexit)
		{
#ifdef _WIN32
			system("pause");
#else
			std::string str;
			std::cout << "Press enter to continue . . .";
			getline(std::cin, str);
#endif
}
#endif
		std::exit(0);
	}

	inline std::string lowercase(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](const char& c) -> char
			{
				if (std::isupper(c))
				{
					return c + 32;
				}
				return c;
			});
		return str;
	}

	inline auto localtime_rs(tm* tm, const time_t* time)
	{
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
		return localtime_r(time, tm);
#elif defined (_MSC_VER)
		return localtime_s(tm, time);
#else
		static_assert(false, "no secure localtime function found, remove this line to proceed with insecure version");
		*tm = std::move(*localtime(time));
#endif
	}

	inline std::string timestamp()
	{
		const time_t timet = time(nullptr);
		tm timeinfo{};
		localtime_rs(&timeinfo, &timet);
		std::string hour = std::to_string(timeinfo.tm_hour);
		if (hour.length() == 1)
		{
			hour.insert(hour.begin(), '0');
		}
		std::string min = std::to_string(timeinfo.tm_min);
		if (min.length() == 1)
		{
			min.insert(min.begin(), '0');
		}
		std::string sec = std::to_string(timeinfo.tm_sec);
		if (sec.length() == 1)
		{
			sec.insert(sec.begin(), '0');
		}
		return '[' + hour + ':' + min + ':' + sec + "] ";
	}

	inline std::string ununderscore(std::string str)
	{
		str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
		return str;
	}

	inline void findreplace(std::string& source, const std::string& find, const std::string& replace)
	{
		long long pos = -static_cast<long long>(replace.size());
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
	}

	inline std::string oftoregex(std::string of)
	{
		findreplace(of, ".", "\\.");
		findreplace(of, "[", "\\[");
		findreplace(of, "]", "\\]");
		findreplace(of, "^", "\\^");
		findreplace(of, "$", "\\$");
		findreplace(of, "+", "\\+");
		findreplace(of, "{", "\\{");
		findreplace(of, "}", "\\}");
		for (size_t i = 0; i < of.size(); i++)
		{
			if (of.at(i) == '*')
			{
				if (i != 0)
				{
					if (of.at(i - 1) == '\\')
					{
						continue;
					}
				}
				of.replace(i, 1, ".*");
				i++;
			}
		}
		for (size_t i = 0; i < of.size(); i++)
		{
			if (of.at(i) == '?')
			{
				if (i != 0)
				{
					if (of.at(i - 1) == '\\')
					{
						continue;
					}
				}
				of.replace(i, 1, ".*");
			}
		}
		return of;
	}

	inline std::atomic_bool waitdontoutput = false; // don't output probably since output is being redrawn

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
		static void print(void* v)
		{
			ui->output->add(static_cast<char*>(v));
			ui->output->bottomline(ui->output->size()); // automatically scroll to the bottom
			delete[] static_cast<char*>(v);
		}
#endif
	public:
#ifdef GUI
		// couldn't find a good pre-defined color for warning
		// this is public so it can be used for redrawing when output level is changed
		static constexpr std::array<Fl_Color, 6> colors = { FL_DARK3, FL_FOREGROUND_COLOR, FL_DARK_GREEN, 92, FL_RED, FL_DARK_MAGENTA };
#endif
		template<typename T>
		outstream operator<<(const T& value) const
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
			return outstream(false, cout, file, err, level);
		}
		outstream operator<<(const std::string& str) const
		{
			if (cout)
			{
#ifdef GUI
				if (first)
				{
					sstream << (dotimestamp ? timestamp() : "");
				}
				sstream << str;
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
					std::cerr << str;
				}
				else
				{
					std::cout << str;
				}
#endif
			}
			if (file && logfile.good())
			{
				if (first)
				{
					logfile << timestamp();
				}
				logfile << str;
			}
			return outstream(false, cout, file, err, level);
		}
		outstream operator<<(std::ostream& (*f)(std::ostream&)) const
		{
			if (cout)
			{
#ifdef GUI
				if (first)
				{
					sstream << (dotimestamp ? timestamp() : "");
				}
				if (f == std::endl<char, std::char_traits<char>>)
				{
					if (sstream.str().empty())
					{
						sstream.str(" "); // fltk won't print empty strings
					}
					while (waitdontoutput)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					// add color and output line
					Fl::awake(print, dupstr(("@S14@C" + std::to_string(colors.at(level - 1)) + "@." + sstream.str())));
					outputted.emplace_back(level, sstream.str()); // we don't need the modifier stuffs since we can add them later on
					sstream.str(std::string());
					sstream.clear();
				}
				else
				{
					sstream << f;
				}
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
					std::cerr << f;
				}
				else
				{
					std::cout << f;
				}
#endif
			}
			if (file && logfile.good())
			{
				if (first)
				{
					logfile << timestamp();
				}
				logfile << f;
			}
			return outstream(false, cout, file, err, level);
		}
	};

	inline outstream out(const short& level) noexcept
	{
		return outstream(true, level >= outputlevel, level >= loglevel, level == 5, level);
	}

	inline void copy(const std::filesystem::path& from, const std::filesystem::path& to)
	{
		if (!std::filesystem::exists(from))
		{
			out(5) << "Error: tried to copy nonexistent file" << std::endl << from.u8string() << std::endl;
			return;
		}
		if (std::filesystem::is_directory(to) != std::filesystem::is_directory(from))
		{
			out(5) << "Error: tried to copy a file to a directory (or vice versa)" << std::endl << from.u8string() << std::endl << to.u8string() << std::endl;
			return;
		}
		if (std::filesystem::exists(to))
		{
			std::filesystem::remove(to);
		}
		try
		{
			std::filesystem::create_directories(to.parent_path());
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			out(5) << "Error creating directory:" << std::endl << e.what() << std::endl;
		}
		try
		{
			std::filesystem::copy(from, to);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			out(5) << "Error copying file:" << std::endl << e.what() << std::endl;
		}
	}

	inline void checkpackver(const std::filesystem::path& path)
	{
		const std::filesystem::path pack_mcmeta = path / "pack.mcmeta"; // kinda weird, this is how you append filesystem paths
		if (!std::filesystem::is_regular_file(pack_mcmeta))
		{
			out(4) << "pack.mcmeta not found; in " << path.filename().u8string() << std::endl;
			return;
		}
		nlohmann::json j;
		std::ifstream fin(pack_mcmeta);
		try
		{
			fin >> j;
			if (j["pack"]["pack_format"].get<int>() != PACK_VER)
			{
				std::stringstream ss;
				ss << "Potentially incorrect pack_format in " << path.filename().u8string() << ". This may cause some resourcepacks to break.\n"
					<< "Version found : " << j["pack"]["pack_format"].get<int>() << "\nLatest version : " << PACK_VER << std::endl;
				out(4) << ss.str();
#ifdef GUI
				wait_close = true;
				const auto alert = [](void* v) { fl_alert(static_cast<char*>(v)); wait_close = false; };
				while (wait_close)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				Fl::awake(alert, const_cast<char*>(ss.str().c_str()));
#endif
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			out(4) << "Json error while parsing pack.mcmeta from " << path.filename().u8string() << ":\n" << e.what() << std::endl;
		}
		fin.close();
	}

	inline void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa)
	{
		zipa.Open(path.u8string());
		std::string folder = path.stem().u8string();
		std::filesystem::create_directories(std::filesystem::u8path("mcpppp-temp/" + folder));
		out(3) << "Extracting " << path.filename().u8string() << std::endl;
		zipa.ExtractAll("mcpppp-temp/" + folder + '/');
	}

	inline void rezip(const std::string& folder, Zippy::ZipArchive& zipa)
	{
		out(3) << "Compressing " + folder << std::endl;
		Zippy::ZipEntryData zed;
		const size_t length = 13 + folder.size();
		size_t filesize;
		for (const auto& png : std::filesystem::recursive_directory_iterator(std::filesystem::u8path("mcpppp-temp/" + folder)))
		{
			if (png.is_directory())
			{
				continue;
			}
			std::ifstream fin(png.path(), std::ios::binary | std::ios::ate);
			zed.clear();
			filesize = png.file_size();
			zed.resize(filesize);
			fin.seekg(0, std::ios::beg);
			fin.read(reinterpret_cast<char*>(zed.data()), static_cast<std::streamsize>(filesize));
			fin.close();
			std::string temp = png.path().generic_u8string();
			temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(length));
			if (temp.front() == '/')
			{
				temp.erase(temp.begin());
			}
			zipa.AddEntry(temp, zed);
		}
		zed.clear();
		zed.shrink_to_fit();
		zipa.Save();
		zipa.Close();
		std::filesystem::remove_all("mcpppp-temp");
	}

	inline void setting(const std::string& option, const nlohmann::json& j)
	{
		if (settings.find(lowercase(option)) == settings.end())
		{
			out(4) << "Unknown setting: " << option << std::endl;
			return;
		}
		const setting_item item = settings.at(lowercase(option));
		const type t = item.setting_type;
		const auto var = item.var;
		if (t == type::boolean)
		{
			try
			{
				std::get<std::reference_wrapper<bool>>(var).get() = j.get<bool>();
			}
			catch (const nlohmann::json::exception&)
			{
				out(5) << "Not a valid value for " << option << ": " << j << "; Expected bool" << std::endl;
			}
		}
		else if (t == type::integer)
		{
			try
			{
				int temp = j.get<int>();
				if (static_cast<bool>(item.min) && static_cast<bool>(item.max) && (temp < item.min || temp > item.max))
				{
					out(5) << "Not a valid value for " << option << ": " << temp << "; Expected integer between " << item.min << " and " << item.max << std::endl;
				}
				std::get<std::reference_wrapper<int>>(var).get() = temp;
			}
			catch (const nlohmann::json::exception&)
			{
				out(5) << "Not a valid value for " << option << ": " << j << "; Expected int" << std::endl;
			}
		}
		else
		{
			try
			{
				std::get<std::reference_wrapper<std::string>>(var).get() = j.get<std::string>();
				if (option == "log")
				{
					dolog = (j.get<std::string>().empty());
					if (dolog)
					{
						logfile.close();
						logfile.open(j.get<std::string>());
					}
				}
			}
			catch (const nlohmann::json::exception&)
			{
				out(5) << "Not a valid value for " << option << ": " << j << "; Expected string" << std::endl;
			}
		}
	}

	inline void readconfig()
	{
		paths.clear();
		if (!config.contains("settings"))
		{
			out(4) << "No settings found" << std::endl;
		}
		else if (config["settings"].type() != nlohmann::json::value_t::object)
		{
			out(5) << "settings must be an object, got " << config["settings"].type_name() << std::endl;
		}
		else
		{
			for (const auto& j : config["settings"].items())
			{
				setting(j.key(), j.value());
			}
		}
		if (!config.contains("paths"))
		{
			out(4) << "No paths found" << std::endl;
			config["paths"] = nlohmann::json::array();
		}
		else if (config["paths"].type() != nlohmann::json::value_t::array)
		{
			out(5) << "paths must be an array, got " << config["paths"].type_name() << std::endl;
			throw std::invalid_argument(std::string("paths must be array, got ") + config["paths"].type_name());
		}
		else
		{
			paths.insert(config["paths"].begin(), config["paths"].end());
		}
		if (config.contains("gui"))
		{
			if (config["gui"].type() == nlohmann::json::value_t::object)
			{
				if (config["gui"].contains("settings"))
				{
					if (config["gui"]["settings"].type() == nlohmann::json::value_t::object)
					{
						for (const auto& j : config["gui"]["settings"].items())
						{
							setting(j.key(), j.value());
						}
					}
				}
				if (config["gui"].contains("paths"))
				{
					if (config["gui"]["paths"].type() == nlohmann::json::value_t::array)
					{
						paths.insert(config["gui"]["paths"].begin(), config["gui"]["paths"].end());
					}
				}
				if (config["gui"].contains("excludepaths"))
				{
					if (config["gui"]["excludepaths"].type() == nlohmann::json::value_t::array)
					{
						for (const std::string& path : config["gui"]["excludepaths"].get<std::vector<std::string>>())
						{
							paths.erase(path);
						}
					}
				}
			}
		}
	}
}
