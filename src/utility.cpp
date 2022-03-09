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

#include "constants.h"
#include "convert.h"

#include "microtar.h"
#include "argparse/argparse.hpp"

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
	static std::atomic_bool wait_close; // wait for dialog to close

	[[noreturn]] void exit()
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

	static auto localtime_rs(tm* tm, const time_t* time)
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
		// std::string::contains in c++23
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
	}

	void findreplace(std::u8string& source, const std::u8string& find, const std::u8string& replace)
	{
		long long pos = -static_cast<long long>(replace.size());
		// std::u8string::contains in C++23
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::u8string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
	}

	std::string c8tomb(const std::u8string& s)
	{
		return std::string(s.begin(), s.end());
	}

	const char* c8tomb(const char8_t* s)
	{
		return reinterpret_cast<const char*>(s);
	}

	std::u8string mbtoc8(const std::string& s)
	{
		return std::u8string(s.begin(), s.end());
	}

	const char8_t* mbtoc8(const char* s)
	{
		return reinterpret_cast<const char8_t*>(s);
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
		if (file && logfile.good())
		{
			if (first)
			{
				logfile << timestamp();
			}
			logfile << str;
		}
#ifdef GUI
		// output to sstream regardless of outputlevel
		if (argc < 2)
		{
			if (first)
			{
				sstream << (dotimestamp ? timestamp() : "");
			}
			sstream << str;
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
				std::cerr << str;
			}
			else
			{
				std::cout << str;
			}
		}
		first = false;
		return *this;
	}

	outstream outstream::operator<<(std::ostream& (*f)(std::ostream&))
	{
#ifdef GUI
		if (argc < 2)
		{
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
				if (cout)
				{
					Fl::awake(print, dupstr(("@S14@C" + std::to_string(colors.at(level - 1)) + "@." + sstream.str())));
				}
				output_mutex.lock();
				outputted.emplace_back(level, sstream.str()); // we don't need the modifier stuffs since we can add them later on
				output_mutex.unlock();
				sstream.str(std::string());
				sstream.clear();
			}
			else
			{
				sstream << f;
			}
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
				std::cerr << f;
			}
			else
			{
				std::cout << f;
			}
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
		return { true, level >= outputlevel, level >= loglevel, level == 5, level };
	}

	void copy(const std::filesystem::path& from, const std::filesystem::path& to)
	{
		if (!std::filesystem::exists(from))
		{
			out(5) << "Error: tried to copy nonexistent file" << std::endl << c8tomb(from.generic_u8string()) << std::endl;
			return;
		}
		if (std::filesystem::is_directory(to) != std::filesystem::is_directory(from))
		{
			out(5) << "Error: tried to copy a file to a directory (or vice versa)" << std::endl << c8tomb(from.generic_u8string()) << std::endl << c8tomb(to.generic_u8string()) << std::endl;
			return;
		}
		if (std::filesystem::exists(to))
		{
			try
			{
				std::filesystem::remove(to);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				out(5) << "Error removing file:" << std::endl << e.what() << std::endl;
			}
		}
		if (!std::filesystem::exists(to.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(to.parent_path());
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				out(5) << "Error creating directory:" << std::endl << e.what() << std::endl;
				return;
			}
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

	static int getpackver(const std::filesystem::path& path)
	{
		const std::filesystem::path pack_mcmeta = path / "pack.mcmeta"; // kinda weird, this is how you append filesystem paths
		if (!std::filesystem::is_regular_file(pack_mcmeta))
		{
			out(4) << "pack.mcmeta not found; in " << c8tomb(path.filename().u8string()) << std::endl;
			return -1;
		}
		nlohmann::json j;
		std::ifstream fin(pack_mcmeta);
		try
		{
			fin >> j;
			if (j["pack"]["pack_format"].get<int>() != PACK_VER)
			{
				return j["pack"]["pack_format"].get<int>();
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			out(4) << "Json error while parsing pack.mcmeta from " << c8tomb(path.filename().u8string()) << ":\n" << e.what() << std::endl;
		}
		fin.close();
		return -1;
	}

	static bool findzipitem(const std::u8string& ziparchive, const std::u8string& itemtofind)
	{
		bool found = false;
		mz_zip_archive archive = mz_zip_archive();
		mz_zip_reader_init_file(&archive, c8tomb(ziparchive.c_str()), 0);
		for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&archive); i++)
		{
			mz_zip_archive_file_stat stat;
			mz_zip_reader_file_stat(&archive, i, &stat);
			if (std::u8string(mbtoc8(stat.m_filename)).starts_with(itemtofind))
			{
				found = true;
				break;
			}
		}
		mz_zip_reader_end(&archive);
		return found;
	}

	bool findfolder(const std::u8string& path, const std::u8string& tofind, const bool zip)
	{
		if (zip)
		{
			return findzipitem(path, tofind);
		}
		else
		{
			return std::filesystem::exists(std::filesystem::path(path + u8'/' + tofind));
		}
	}

	static void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa)
	{
		out(3) << "Extracting " << c8tomb(path.filename().u8string()) << std::endl;
		zipa.Open(c8tomb(path.generic_u8string()));
		const std::u8string folder = path.stem().generic_u8string();
		std::filesystem::create_directories(std::filesystem::path(u8"mcpppp-temp/" + folder));
		zipa.ExtractAll("mcpppp-temp/" + c8tomb(folder) + '/');
	}

	static void rezip(const std::u8string& folder, Zippy::ZipArchive& zipa)
	{
		out(3) << "Compressing " + c8tomb(folder) << std::endl;
		Zippy::ZipEntryData zed;
		const size_t length = 13 + folder.size();
		size_t filesize;
		for (const auto& png : std::filesystem::recursive_directory_iterator(std::filesystem::path(u8"mcpppp-temp/" + folder)))
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
			std::u8string temp = png.path().generic_u8string();
			temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(length));
			if (temp.front() == '/')
			{
				temp.erase(temp.begin());
			}
			zipa.AddEntry(c8tomb(temp), zed);
		}
		zed.clear();
		zed.shrink_to_fit();
		zipa.Save();
		zipa.Close();
		std::filesystem::remove_all("mcpppp-temp");
	}

	static std::vector<std::pair<std::filesystem::directory_entry, std::u8string>> items;

	static std::uintmax_t getitems(const std::filesystem::path& path)
	{
		std::uintmax_t size = 0;
		items.clear();
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
		{
			// I'd prefer to use std::filesystem::relative, but it's too slow when we have thousands of files
			std::u8string name = entry.path().generic_u8string();
			name.erase(name.begin(), name.begin() + path.generic_u8string().size() + 1);

			items.emplace_back(entry, name);
			if (entry.is_regular_file())
			{
				size += entry.file_size();
			}
		}
		return size;
	}

	static std::string hash(const std::filesystem::path& path, const bool zip)
	{
		out(3) << "Computing Hash: " << c8tomb(path.filename().u8string()) << std::endl;
		if (zip)
		{
			const std::uintmax_t filesize = std::filesystem::file_size(path);
			std::vector<char> file_contents(filesize);
			std::ifstream fin(path);
			fin.read(file_contents.data(), filesize);
			fin.close();
			return hash<128>(file_contents.data(), file_contents.size());
		}
		else
		{
			// create tar of directory, which we then hash
			mtar_t tar = mtar_t();
			mtar_mem_stream_t mem = mtar_mem_stream_t();
			mtar_init_mem_stream(&mem);
			mtar_open_mem(&tar, &mem);
			// get items and also reserve most of the space needed
			mem.data.reserve(getitems(path));
			std::sort(items.begin(), items.end());
			for (const auto& item : items)
			{
				if (item.first.is_directory())
				{
					mtar_write_dir_header(&tar, c8tomb(item.second.c_str()));
				}
				else
				{
					const std::uintmax_t filesize = item.first.file_size();
					std::vector<char> file_contents(filesize);
					std::ifstream fin(item.first.path(), std::ios::in | std::ios::binary);
					fin.read(file_contents.data(), filesize);
					fin.close();
					mtar_write_file_header(&tar, c8tomb(item.second.c_str()), filesize);
					mtar_write_data(&tar, file_contents.data(), filesize);
				}
			}
			mtar_finalize(&tar);
			std::string hashvalue = hash<128>(mem.data.data(), mem.data.size());
			mtar_close(&tar);
			return hashvalue;
		}
	}

	// convert a single folder/file
	bool convert(const std::filesystem::path& path, const bool dofsb, const bool dovmt, const bool docim)
	{
		if (!std::filesystem::is_directory(path) && path.extension() != ".zip")
		{
			out(5) << "Tried to convert invalid pack:" << std::endl << c8tomb(path.generic_u8string());
			return false;
		}
		const bool zip = (path.extension() == ".zip");
		const checkinfo fsb = fsb::check(path, zip), vmt = vmt::check(path, zip), cim = cim::check(path, zip);
		bool reconvert = autoreconvert;
		// TODO: Do we really need to delete before reconversion?
		Zippy::ZipArchive zipa;
		const std::u8string folder = path.stem().generic_u8string();
		std::string hashvalue, namehash = hash<64>(c8tomb(path.generic_u8string()).data(), path.generic_u8string().size());
		std::u8string convert;
		const auto isvalid = [](const checkinfo& info, const bool& doconversion, const bool& strict = false) -> bool
		{
			if (!doconversion)
			{
				return false;
			}
			if (strict)
			{
				return info.results == checkresults::valid;
			}
			else
			{
				return (info.results == checkresults::valid || info.results == checkresults::reconverting);
			}
		};
		const auto isreconvert = [](const checkinfo& info, const bool& doconversion) -> bool
		{
			if (!doconversion || !autoreconvert)
			{
				return false;
			}
			return info.results == checkresults::reconverting;
		};

		// only compute hash if possibly reconverting
		if (isreconvert(fsb, dofsb) || isreconvert(vmt, dovmt) || isreconvert(cim, docim))
		{
			// no point of calculating hash if we have nothing to compare it to
			if (hashes.contains(namehash))
			{
				hashvalue = hash(path, zip);
				if (hashes[namehash] != hashvalue)
				{
					out(2) << "Pack appears to have changed: " << c8tomb(path.filename().u8string()) << ", reconverting" << std::endl;
				}
				else
				{
					// don't reconvert if pack hasn't changed
					reconvert = false;
				}
			}
			else
			{
				// don't reconvert packs that haven't been previously hashed,
				// which could prevent data loss
				reconvert = false;
			}
		}

		// if we aren't reconverting, reconverting result is not valid
		// if we are reconverting, it is valid
		if (isvalid(fsb, dofsb, !reconvert) || isvalid(vmt, dovmt, !reconvert) || isvalid(cim, docim, !reconvert))
		{
			if (zip)
			{
				unzip(path, zipa);
				convert = u8"mcpppp-temp/" + folder;
			}
			else
			{
				convert = path.generic_u8string();
			}
		}

		if (dofsb)
		{
			switch (fsb.results)
			{
			case checkresults::valid:
				fsb::convert(convert, path.filename().u8string(), fsb);
				break;
			case checkresults::noneconvertible:
				out(2) << "FSB: Nothing to convert in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::alrfound:
				out(2) << "FSB: Fabricskyboxes folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "FSB: Reconverting " << c8tomb(path.filename().u8string()) << std::endl;
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/fabricskyboxes"));
					fsb::convert(convert, path.filename().u8string(), fsb);
				}
				else
				{
					out(2) << "FSB: Fabricskyboxes folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				}
				break;
			}
		}
		if (dovmt)
		{
			switch (vmt.results)
			{
			case checkresults::valid:
				vmt::convert(convert, path.filename().u8string(), vmt);
				break;
			case checkresults::noneconvertible:
				out(2) << "VMT: Nothing to convert in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::alrfound:
				out(2) << "VMT: Varied Mob Textures folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "VMT: Reconverting " << c8tomb(path.filename().u8string()) << std::endl;
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/minecraft/varied/"));
					vmt::convert(convert, path.filename().u8string(), vmt);
				}
				else
				{
					out(2) << "VMT: Varied Mob Textures folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				}
				break;
			}
		}
		if (docim)
		{
			switch (cim.results)
			{
			case checkresults::valid:
				cim::convert(convert, path.filename().u8string(), cim);
				break;
			case checkresults::noneconvertible:
				out(2) << "CIM: Nothing to convert in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::alrfound:
				out(2) << "CIM: Chime folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "CIM: Reconverting " << c8tomb(path.filename().u8string()) << std::endl;
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/mcpppp"));
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/minecraft/overrides"));
					cim::convert(convert, path.filename().u8string(), cim);
				}
				else
				{
					out(2) << "CIM: Chime folder found in " << c8tomb(path.filename().u8string()) << ", skipping" << std::endl;
				}
				break;
			}
		}

		// put it here, because we want to output the error messages if it isn't valid
		if (!isvalid(fsb, dofsb, !reconvert) && !isvalid(vmt, dovmt, !reconvert) && !isvalid(cim, docim, !reconvert))
		{
			return false;
		}

		const int packver = getpackver(convert);
		if (zip)
		{
			rezip(folder, zipa);
		}

		hashvalue = hash(path, zip);
		hashes[namehash] = hashvalue;
		savehashes();

		if (packver != -1)
		{
			// output it again since it doesn't like \n or something
			out(4) << "Potentially incorrect pack_format in " << c8tomb(path.filename().u8string()) << ". This may cause some resourcepacks to break." << std::endl
				<< "Version found : " << packver << std::endl
				<< "Latest version : " << PACK_VER << std::endl;
#ifdef GUI
			char* c;
			{
				std::stringstream ss;
				ss << "Potentially incorrect pack_format in " << c8tomb(path.filename().u8string()) << ". This may cause some resourcepacks to break.\n"
					<< "Version found : " << packver << "\nLatest version : " << PACK_VER;
				c = dupstr(ss.str());
			}
			wait_close = true;
			const auto alert = [](void* v) { fl_alert(static_cast<char*>(v)); wait_close = false; };
			Fl::awake(alert, c);
			while (wait_close)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			delete[] c;
#endif
		}

		return true;
	}

	void gethashes()
	{
		if (!std::filesystem::exists("mcpppp-hashes.json"))
		{
			// create file with empty json
			std::ofstream emptyfile("mcpppp-hashes.json");
			emptyfile << "{}" << std::endl;
			emptyfile.close();
		}
		else
		{
			std::uintmax_t filesize = std::filesystem::file_size("mcpppp-hashes.json");
			std::ifstream hashfile("mcpppp-hashes.json");
			std::vector<char> contents(filesize);
			hashfile.read(contents.data(), filesize);
			hashfile.close();
			hashes = nlohmann::json::parse(contents);
		}
	}

	void savehashes()
	{
		std::ofstream hashfile("mcpppp-hashes.json");
		hashfile << hashes.dump(1, '\t') << std::endl;
		hashfile.close();
	}

	void setting(const std::string& option, const nlohmann::json& j)
	{
		if (!settings.contains(lowercase(option)))
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
				if (item.min != 0 && item.max != 0 && (temp < item.min || temp > item.max))
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
			for (auto it = config["paths"].begin(); it != config["paths"].end(); it++)
			{
				// check that path exists, otherwise canonical will fail
				if (std::filesystem::exists(mbtoc8(*it)))
				{
					paths.insert(std::filesystem::canonical(mbtoc8(*it)));
				}
				else
				{
					out(4) << "Invalid path: " << *it << std::endl;
				}
			}
		}
#ifdef GUI
		if (config.contains("gui"))
		{
			if (config["gui"].type() == nlohmann::json::value_t::object)
			{
				if (config["gui"].contains("settings") && config["gui"]["settings"].type() == nlohmann::json::value_t::object)
				{
					for (const auto& j : config["gui"]["settings"].items())
					{
						setting(j.key(), j.value());
					}
				}
				if (config["gui"].contains("paths") && config["gui"]["paths"].type() == nlohmann::json::value_t::array)
				{
					for (auto it = config["gui"]["paths"].begin(); it != config["gui"]["paths"].end(); it++)
					{
						// check that path exists, otherwise canonical will fail
						if (std::filesystem::exists(mbtoc8(*it)))
						{
							paths.insert(std::filesystem::canonical(mbtoc8(*it)));
						}
						else
						{
							out(4) << "Invalid path: " << *it << std::endl;
						}
					}
				}
				if (config["gui"].contains("excludepaths") && config["gui"]["excludepaths"].type() == nlohmann::json::value_t::array)
				{
					for (const std::string& path : config["gui"]["excludepaths"].get<std::vector<std::string>>())
					{
						paths.erase(mbtoc8(path));
					}
				}
			}
		}
#endif
	}

	void parseargs(int argc, const char* argv[])
	{
		argparse::ArgumentParser parser("MCPPPP", VERSION, argparse::default_arguments::help);
		// default one also has -v, which we are using as verbose
		parser.add_argument("--version")
			.help("prints version information and exits")
			.action([&](const auto& a) { std::cout << "MCPPPP " << VERSION; std::exit(0); })
			.nargs(0)
			.default_value(false)
			.implicit_value(true);

#ifdef GUI
		parser.add_epilog("If no command line arguments are provided, the normal GUI may be used");
#else
		parser.add_epilog("If no command line arguments are provided, the config file may be used. The documentation for that can be found at https://github.com/supsm/MCPPPP/blob/master/CONFIG.md");
#endif

		parser.add_argument("-v", "--verbose")
			.help("Outputs more information (can be used upto 2 times)")
			.action([](const auto&) { if (outputlevel > 1) { outputlevel--; } })
			.nargs(0)
			.default_value(false)
			.implicit_value(true)
			.append();

		parser.add_argument("--pauseOnExit")
			.help("Wait for enter key to be pressed once execution has been finished")
			.default_value(std::string(pauseonexit ? "true" : "false"));
		parser.add_argument("--log")
			.help("Log file where logs will be stored")
			.default_value(logfilename);
		parser.add_argument("--timestamp")
			.help("Add timestamp to output")
			.default_value(std::string(dotimestamp ? "true" : "false"));
		parser.add_argument("--autoDeleteTemp")
			.help("Automatically delete `mcpppp-temp` folder on startup")
			.default_value(std::string(autodeletetemp ? "true" : "false"));
		parser.add_argument("--autoReconvert")
			.help("Automatically reconvert resourcepacks instead of skipping. Could lose data if a pack isn't converted with MCPPPP")
			.default_value(std::string(autoreconvert ? "true" : "false"));
		parser.add_argument("--fsbTransparent")
			.help("Make Fabricskyboxes skyboxes semi-transparent to replicate what optifine does internally")
			.default_value(std::string(fsbtransparent ? "true" : "false"));

		parser.add_argument("resourcepacks")
			.help("The resourcepacks to convert")
			.remaining();

		try
		{
			parser.parse_args(argc, argv);
		}
		catch (const std::runtime_error& e)
		{
			out(5) << e.what() << std::endl;
			std::exit(-1);
		}

		const auto truefalse = [](const std::string& str) -> bool
		{
			if (lowercase(str) == "true")
			{
				return true;
			}
			else if (lowercase(str) == "false")
			{
				return false;
			}
			else
			{
				out(4) << "Unrecognized value (expected true, false): " << str << std::endl << "Interpreting as false" << std::endl;
				return false;
			}
		};

		pauseonexit = truefalse(parser.get<std::string>("--pauseOnExit"));
		if (parser.is_used("--log"))
		{
			logfilename = parser.get<std::string>("--log");
			logfile.open(logfilename);
		}
		dotimestamp = truefalse(parser.get<std::string>("--timestamp"));
		autodeletetemp = truefalse(parser.get<std::string>("--autoDeleteTemp"));
		autoreconvert = truefalse(parser.get<std::string>("--autoReconvert"));
		fsbtransparent = truefalse(parser.get<std::string>("--fsbTransparent"));

		try
		{
			auto resourcepacks = parser.get<std::vector<std::string>>("resourcepacks");
			resourcepacks.erase(
				std::remove_if(resourcepacks.begin(), resourcepacks.end(), [](const std::string& s) -> bool
					{
						if (!std::filesystem::exists(s))
						{
							out(5) << "Invalid pack: " << s << std::endl;
							return true;
						}
						return false;
					}), resourcepacks.end());
			std::transform(resourcepacks.begin(), resourcepacks.end(), std::back_inserter(mcpppp::entries), [](const std::string& s)
				{
					return std::make_pair(true, std::filesystem::directory_entry(s));
				});
		}
		catch (const std::logic_error& e)
		{
			out(5) << e.what() << std::endl;
			std::exit(0);
		}
	}
}
