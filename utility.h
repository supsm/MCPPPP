/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <algorithm>
#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json.hpp"

#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif

#include "Zippy.hpp"

inline bool autodeletetemp = false, pauseonexit = true, dolog = true, dotimestamp = false, autoreconvert = false;
inline int outputlevel = 3, loglevel = 1;
inline std::ofstream logfile("mcpppp-log.txt");
static std::string logfilename = "mcpppp-log.txt";

inline std::set<std::string> paths = {};
inline nlohmann::ordered_json config;

enum class type { boolean, integer, string };

// type, pointer to variable to modify, default value
const std::unordered_map<std::string, std::tuple<type, std::variant<bool*, int*, std::string*>, nlohmann::json>> settings =
{
	{"pauseonexit", {type::boolean, &pauseonexit, pauseonexit}},
	{"log", {type::string, &logfilename, logfilename}},
	{"timestamp", {type::boolean, &dotimestamp, dotimestamp}},
	{"autodeletetemp", {type::boolean, &autodeletetemp, autodeletetemp}},
	{"outputlevel", {type::integer, &outputlevel, outputlevel}},
	{"loglevel", {type::integer, &loglevel, loglevel}},
	{"autoreconvert", {type::boolean, &autoreconvert, autoreconvert}}
};

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
#include "mcpppp.cxx" // ui class

static std::stringstream sstream;
static Fl_Text_Buffer textbuffer;
inline std::unique_ptr<UI> ui = std::make_unique<UI>();
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
	exit(0);
}

inline std::string lowercase(std::string str) noexcept
{
	for (char& c : str)
	{
		if (c >= 'A' && c <= 'Z')
		{
			c += 32;
		}
	}
	return str;
}

inline std::string timestamp()
{
	const time_t timet = time(nullptr);
	const tm* timeinfo = localtime(&timet);
	std::string hour = std::to_string(timeinfo->tm_hour);
	if (hour.length() == 1)
	{
		hour.insert(hour.begin(), '0');
	}
	std::string min = std::to_string(timeinfo->tm_min);
	if (min.length() == 1)
	{
		min.insert(min.begin(), '0');
	}
	std::string sec = std::to_string(timeinfo->tm_sec);
	if (sec.length() == 1)
	{
		sec.insert(sec.begin(), '0');
	}
	return '[' + hour + ':' + min + ':' + sec + "] ";
}

inline std::string ununderscore(std::string& str)
{
	std::string str2;
	for (const char& c : str)
	{
		if (c != '_')
		{
			str2 += c;
		}
	}
	return str2;
}

inline std::string wtomb(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(str);
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

static bool cout, file, err;

class outstream
{
public:
	bool first = false;
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
			textbuffer.text(sstream.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
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
		return outstream();
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
			textbuffer.text(sstream.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
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
		return outstream();
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
			sstream << f;
			textbuffer.text(sstream.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
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
		return outstream();
	}
};

inline outstream out(const int level) noexcept
{
	outstream o;
	o.first = true;
	err = (level == 5);
	cout = (level >= outputlevel);
	file = (level >= loglevel);
	return o;
}

namespace supsm
{
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
}

inline void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa)
{
	zipa.Open(path.u8string());
	std::string folder = path.stem().u8string();
	std::filesystem::create_directories("mcpppp-temp/" + folder);
	out(3) << "Extracting " << path.filename().u8string() << std::endl;
	zipa.ExtractAll("mcpppp-temp/" + folder + '/');
}

inline void rezip(const std::string& folder, Zippy::ZipArchive& zipa)
{
	out(3) << "Compressing " + folder << std::endl;
	Zippy::ZipEntryData zed;
	const size_t length = 13 + folder.size();
	size_t filesize;
	for (const auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder))
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
		temp.erase(temp.begin(), temp.begin() + length);
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
	const type t = std::get<0>(settings.at(lowercase(option)));
	const auto var = std::get<1>(settings.at(lowercase(option)));
	if (t == type::boolean)
	{
		try
		{
			*(std::get<bool*>(var)) = j.get<bool>();
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
			*(std::get<int*>(var)) = j.get<int>();
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
			*(std::get<std::string*>(var)) = j.get<std::string>();
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
