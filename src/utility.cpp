/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// VERSION and PACK_VER are in utility.h

#include "utility.h"

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

#include "convert.h"

#ifdef GUI
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>

namespace mcpppp
{
	static Fl_Text_Buffer textbuffer;
}
#endif

namespace mcpppp
{
	std::atomic_bool wait_close; // wait for dialog to close

	[[noreturn]] void exit() noexcept
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

	std::string lowercase(std::string str)
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

	auto localtime_rs(tm* tm, const time_t* time)
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

	std::string timestamp()
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

	std::string ununderscore(std::string str)
	{
		str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
		return str;
	}

	void findreplace(std::string& source, const std::string& find, const std::string& replace)
	{
		long long pos = -static_cast<long long>(replace.size());
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
	}

	std::string oftoregex(std::string of)
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

#ifdef GUI
	void outstream::print(void* v)
	{
		ui->output->add(static_cast<char*>(v));
		ui->output->bottomline(ui->output->size()); // automatically scroll to the bottom
		delete[] static_cast<char*>(v);
	}
#endif

	outstream outstream::operator<<(const std::string& str)
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
		first = false;
		return *this;
	}

	outstream outstream::operator<<(std::ostream& (*f)(std::ostream&))
	{
		if (cout)
		{
#ifdef GUI
			if (first)
			{
				sstream << (dotimestamp ? timestamp() : "");
			}
			if (f == static_cast<std::basic_ostream<char>&(*)(std::basic_ostream<char>&)>(&std::endl))
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
		first = false;
		return *this;
	}

	outstream out(const short& level) noexcept
	{
		return outstream(true, level >= outputlevel, level >= loglevel, level == 5, level);
	}

	void copy(const std::filesystem::path& from, const std::filesystem::path& to)
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

	void checkpackver(const std::filesystem::path& path)
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
				Fl::awake(alert, const_cast<char*>(ss.str().c_str()));
				while (wait_close)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
#endif
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			out(4) << "Json error while parsing pack.mcmeta from " << path.filename().u8string() << ":\n" << e.what() << std::endl;
		}
		fin.close();
	}

	bool findzipitem(const std::string& ziparchive, const std::string& itemtofind)
	{
		bool found = false;
		mz_zip_archive archive = mz_zip_archive();
		mz_zip_reader_init_file(&archive, ziparchive.c_str(), 0);
		for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&archive); i++)
		{
			mz_zip_archive_file_stat stat;
			mz_zip_reader_file_stat(&archive, i, &stat);
			if (std::string(stat.m_filename).rfind(itemtofind, 0) == 0)
			{
				found = true;
				break;
			}
		}
		mz_zip_reader_end(&archive);
		return found;
	}

	bool findfolder(const std::string& path, const std::string& tofind, const bool& zip)
	{
		if (zip)
		{
			return findzipitem(path, tofind);
		}
		else
		{
			return std::filesystem::exists(std::filesystem::u8path(path + '/' + tofind));
		}
	}

	void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa)
	{
		zipa.Open(path.u8string());
		std::string folder = path.stem().u8string();
		std::filesystem::create_directories(std::filesystem::u8path("mcpppp-temp/" + folder));
		out(3) << "Extracting " << path.filename().u8string() << std::endl;
		zipa.ExtractAll("mcpppp-temp/" + folder + '/');
	}

	void rezip(const std::string& folder, Zippy::ZipArchive& zipa)
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

	// convert a single folder/file
	bool convert(const std::filesystem::path& path, const bool& dofsb, const bool& dovmt, const bool& docim)
	{
		mcpppp::checkinfo info = {false, false, false};
		bool success = false;
		if (std::filesystem::is_directory(path))
		{
			if (dofsb)
			{
				info = fsb::check(path, false);
				if (info.convert)
				{
					fsb::convert(path.u8string(), path.filename().u8string(), info);
					success = true;
				}
			}
			if (dovmt)
			{
				info = vmt::check(path, false);
				if (info.convert)
				{
					vmt::convert(path.u8string(), path.filename().u8string(), info);
					success = true;
				}
			}
			if (docim)
			{
				info = cim::check(path, false);
				if (info.convert)
				{
					cim::convert(path.u8string(), path.filename().u8string(), info);
					success = true;
				}
			}
			if (success)
			{
				checkpackver(path);
			}
		}
		else if (path.extension() == ".zip")
		{
			Zippy::ZipArchive zipa;
			const std::string folder = path.stem().u8string();
			if (dofsb)
			{
				info = fsb::check(path, true);
				if (info.convert)
				{
					if (!success)
					{
						unzip(path.u8string(), zipa);
					}
					fsb::convert("mcpppp-temp/" + folder, path.filename().u8string(), info);
					success = true;
				}
			}
			if (dovmt)
			{
				info = vmt::check(path, true);
				if (info.convert)
				{
					vmt::convert("mcpppp-temp/" + folder, path.filename().u8string(), info);
					success = true;
				}
			}
			if (docim)
			{
				info = cim::check(path, true);
				if (info.convert)
				{
					cim::convert("mcpppp-temp/" + folder, path.filename().u8string(), info);
					success = true;
				}
			}
			if (success)
			{
				checkpackver("mcpppp-temp/" + folder);
				rezip(folder, zipa);
			}
		}
		return success;
	}

	void setting(const std::string& option, const nlohmann::json& j)
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

	void readconfig()
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
