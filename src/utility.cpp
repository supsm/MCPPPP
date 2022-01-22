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

#include "microtar.h"
#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "xxhash.h"

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
			if (argc < 2)
			{
				if (first)
				{
					sstream << (dotimestamp ? timestamp() : "");
				}
				sstream << str;
			}
			else
#endif
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
					Fl::awake(print, dupstr(("@S14@C" + std::to_string(colors.at(level - 1)) + "@." + sstream.str())));
					outputted.emplace_back(level, sstream.str()); // we don't need the modifier stuffs since we can add them later on
					sstream.str(std::string());
					sstream.clear();
				}
				else
				{
					sstream << f;
				}
			}
			else
#endif
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
					<< "Version found : " << j["pack"]["pack_format"].get<int>() << "\nLatest version : " << PACK_VER;
				// output it again since it doesn't like \n or something
				out(4) << "Potentially incorrect pack_format in " << path.filename().u8string() << ". This may cause some resourcepacks to break." << std::endl
					<< "Version found : " << j["pack"]["pack_format"].get<int>() << std::endl
					<< "Latest version : " << PACK_VER << std::endl;
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

	static bool findzipitem(const std::string& ziparchive, const std::string& itemtofind)
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

	static std::vector<std::pair<std::filesystem::directory_entry, std::string>> items;

	static std::uintmax_t getitems(const std::filesystem::path& path)
	{
		std::uintmax_t size = 0;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
		{
			const std::string name = std::filesystem::relative(entry.path(), path).generic_u8string();
			items.emplace_back(entry, name);
			if (entry.is_regular_file())
			{
				size += entry.file_size();
			}
		}
		return size;
	}

	static std::string gethex(const XXH128_hash_t& rawhash)
	{
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(16) << std::hex << rawhash.high64 << rawhash.low64;
		return ss.str();
	}

	static std::string hash(const std::vector<char>& v)
	{
		const XXH128_hash_t rawhash = XXH128(v.data(), v.size(), 0);
		return gethex(rawhash);
	}

	static std::string hash(const std::filesystem::path& path, const bool& zip)
	{
		if (zip)
		{
			const std::uintmax_t filesize = std::filesystem::file_size(path);
			std::vector<char> file_contents(filesize);
			std::ifstream fin(path);
			fin.read(file_contents.data(), filesize);
			fin.close();
			return hash(file_contents);
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
			for (const auto& item : items)
			{
				if (item.first.is_directory())
				{
					mtar_write_dir_header(&tar, item.second.c_str());
				}
				else
				{
					const std::uintmax_t filesize = item.first.file_size();
					std::vector<char> file_contents(filesize);
					std::ifstream fin(item.first.path());
					fin.read(file_contents.data(), filesize);
					fin.close();
					mtar_write_file_header(&tar, item.second.c_str(), filesize);
					mtar_write_data(&tar, file_contents.data(), filesize);
				}
			}
			mtar_finalize(&tar);
			std::string hashvalue = hash(mem.data);
			mtar_close(&tar);
			return hashvalue;
		}
	}

	// convert a single folder/file
	bool convert(const std::filesystem::path& path, const bool& dofsb, const bool& dovmt, const bool& docim)
	{
		if (!std::filesystem::is_directory(path) && path.extension() != ".zip")
		{
			out(5) << "Tried to convert invalid pack:" << std::endl << path.u8string();
			return false;
		}
		const bool zip = (path.extension() == ".zip");
		const checkinfo fsb = fsb::check(path, zip), vmt = vmt::check(path, zip), cim = cim::check(path, zip);
		bool reconvert = autoreconvert;
		// TODO: Do we really need to delete before reconversion?
		Zippy::ZipArchive zipa;
		const std::string folder = path.stem().u8string();
		std::string convert, hashvalue;
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

		// skip hashing if none are valid
		if (isvalid(fsb, dofsb) || isvalid(vmt, dovmt) || isvalid(cim, docim))
		{
			hashvalue = hash(path, zip);
			if (hashes.contains(path.generic_u8string()) && hashes[path.generic_u8string()] != hashvalue)
			{
				out(2) << "Pack appears to have changed: " << path.filename().u8string() << ", reconverting" << std::endl;
			}
			else
			{
				// don't reconvert if pack isn't changed
				// also has the added benefit of not reconverting packs that haven't been previously hashed,
				// which could prevent data loss
				reconvert = false;
			}

			// if we aren't reconverting, reconverting result is not valid
			// if we are reconverting, it is valid
			if (isvalid(fsb, dofsb, !reconvert) || isvalid(vmt, dovmt, !reconvert) || isvalid(cim, docim, !reconvert))
			{
				if (zip)
				{
					unzip(path, zipa);
					convert = "mcpppp-temp/" + folder;
				}
				else
				{
					convert = path.u8string();
				}
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
				out(2) << "FSB: Nothing to convert in " << path.filename().u8string() << ", skipping" << std::endl;
				break;
			case checkresults::alrfound:
				out(2) << "FSB: Fabricskyboxes folder found in " << path.filename().u8string() << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "FSB: Reconverting " << path.filename().u8string() << std::endl;
					std::filesystem::remove_all(std::filesystem::u8path(convert + "/assets/fabricskyboxes"));
					fsb::convert(convert, path.filename().u8string(), fsb);
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
				out(2) << "VMT: Nothing to convert in " << path.filename().u8string() << ", skipping" << std::endl;
				break;
			case checkresults::alrfound:
				out(2) << "VMT: Varied Mob Textures folder found in " << path.filename().u8string() << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "VMT: Reconverting " << path.filename().u8string() << std::endl;
					std::filesystem::remove_all(std::filesystem::u8path(convert + "/assets/minecraft/varied/"));
					vmt::convert(convert, path.filename().u8string(), vmt);
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
				break;
			case checkresults::alrfound:
				out(2) << "CIM: Chime folder found in " << path.filename().u8string() << ", skipping" << std::endl;
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					out(3) << "CIM: Reconverting " << path.filename().u8string() << std::endl;
					std::filesystem::remove_all(std::filesystem::u8path(convert + "/assets/mcpppp"));
					std::filesystem::remove_all(std::filesystem::u8path(convert + "/assets/minecraft/overrides"));
					cim::convert(convert, path.filename().u8string(), cim);
				}
				break;
			}
		}

		// put it here, because we want to output the error messages if it isn't valid
		if (!isvalid(fsb, dofsb, !reconvert) && !isvalid(vmt, dovmt, !reconvert) && !isvalid(cim, docim, !reconvert))
		{
			return false;
		}

		checkpackver(convert);
		if (zip)
		{
			rezip(folder, zipa);
		}

		hashvalue = hash(path, zip);
		hashes[path.generic_u8string()] = hashvalue;
		savehashes();
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
#ifdef GUI
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
#endif
	}

	void parseargs(int argc, const char* argv[])
	{
		argparse::ArgumentParser parser("MCPPPP", VERSION, argparse::default_arguments::help);
		// default one also has -v, which we are using as verbose
		parser.add_argument("--version")
			.help("prints version information and exits")
			.action([&](const auto&)
				{
					std::cout << "MCPPPP " << VERSION;
					std::exit(0);
				})
			.nargs(0)
			.default_value(false)
			.implicit_value(true);

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
