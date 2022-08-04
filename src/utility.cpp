/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

 // VERSION and PACK_VER are in utility.h

#include "pch.h"

#include "utility.h"

#include "constants.h"
#include "convert.h"

#ifdef GUI
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#endif

namespace mcpppp
{
	// secure version of localtime
	static auto localtime_rs(tm* tm, const time_t* time) noexcept
	{
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
		return localtime_r(time, tm);
#elif defined (_MSC_VER)
		return localtime_s(tm, time);
#else
		// WARNING: insecure!!
		* tm = std::move(*localtime(time));
#endif
	}

	std::string timestamp() noexcept
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
		return fmt::format("[{}:{}:{}]", hour, min, sec);
	}

	void findreplace(std::string& source, const std::string_view& find, const std::string_view& replace)
	{
		long long pos = -static_cast<long long>(replace.size());
		// std::string::contains in c++23
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
		checkpoint();
	}

	void findreplace(std::u8string& source, const std::u8string_view& find, const std::u8string_view& replace)
	{
		long long pos = -static_cast<long long>(replace.size());
		// std::u8string::contains in C++23
		while (source.find(find, static_cast<size_t>(pos) + replace.size()) != std::u8string::npos)
		{
			pos = static_cast<long long>(source.find(find, static_cast<size_t>(pos) + replace.size()));
			source.replace(static_cast<size_t>(pos), find.length(), replace);
		}
		checkpoint();
	}

	namespace conv
	{
		std::string ununderscore(std::string str)
		{
			str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
			checkpoint();
			return str;
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
			checkpoint();
			return of;
		}

		std::string getfilenamehash(const std::filesystem::path& path, const bool zip)
		{
			const std::u8string u8s = path.filename().u8string() + (zip ? u8".zip" : u8"");
			checkpoint();
			return mcpppp::hash<32>(u8s.data(), u8s.size());
		}

		std::unordered_map<std::string, std::string> parse_properties(const std::string_view& data)
		{
			std::string line;
			std::istringstream ss{ std::string(data) };
			std::unordered_map<std::string, std::string> m;

			bool isvalue = false;
			bool delim_is_space = false;
			bool skipline = false; // skip current line
			bool escape = false; // escape next character
			bool unicode = false;
			std::string utfcharhex, key, value;
			const auto add = [&isvalue, &key, &value](const char c) -> void
			{
				if (isvalue)
				{
					value += c;
				}
				else
				{
					key += c;
				}
			};

			while (std::getline(ss, line))
			{
				// deal with CR in CRLF on unix systems
				if (line.ends_with('\r'))
				{
					line.pop_back();
				}
				if (!escape) // keep original things when escaping newline
				{
					isvalue = false;
					delim_is_space = false;
					skipline = false;
					key.clear();
					value.clear();
				}
				escape = false; // remove escape
				bool blank = true; // ignore all blank characters in the beginning of a line

				for (const char& c : line)
				{
					if (escape)
					{
						switch (c)
						{
						case '=':
						case ':':
						case ' ':
						case '\t': // usually delimiting characters, when escaped just add them

						case '#':
						case '!': // usually comments, when escaped just add them

						case '\\': // backslash character
							add(c);
							break;
						case 'n':
							add('\n'); // newline
							break;
						case 't':
							add('\t'); // tab
							break;
						case 'r':
							add('\r'); // carriage return
							break;
						case 'f':
							add('\f'); // form feed (i have no idea what this does)
							break;
						case 'u':
							unicode = true; // unicode
							utfcharhex.clear();
							break;
						default:
							add(c);
						}
						escape = false;
						continue;
					}
					else if (unicode)
					{
						utfcharhex += c;
						if (utfcharhex.size() >= 4)
						{
							char16_t c16 = std::stoi(utfcharhex, nullptr, 16);

							auto cptoutf8 = [](const char32_t c) -> std::string
							{
								if (c < 0x80)
								{
									// 1 byte (from 7 bits)
									return std::string{ static_cast<char>(c) };
								}
								else if (c < 0x800)
								{
									// 2 bytes (from 11 bits)
									char char1 = 0b11000000 | ((c >> 6) & 0b11111); // first 5 bits
									char char2 = 0b10000000 | (c & 0b111111); // last 6 bits
									return std::string{ char1, char2 };
								}
								else if (c < 0x10000)
								{
									// 3 bytes (from 16 bits)
									char char1 = 0b11100000 | ((c >> 12) & 0b1111); // first 4 bits
									char char2 = 0b10000000 | ((c >> 6) & 0b111111); // bits 5-10
									char char3 = 0b10000000 | (c & 0b111111); // last 6 bits
									return std::string{ char1, char2, char3 };
								}
								else if (c < 0x110000)
								{
									// 4 bytes (from 21 bits)
									char char1 = 0b11110000 | ((c >> 18) & 0b111); // first 3 bits
									char char2 = 0b10000000 | ((c >> 12) & 0b111111); // bits 4-9
									char char3 = 0b10000000 | ((c >> 6) & 0b111111); // bits 10-15
									char char4 = 0b10000000 | (c & 0b111111); // last 6 bits
									return std::string{ char1, char2, char3, char4 };
								}
								else
								{
									throw std::invalid_argument("Invalid Unicode Code Point (too large)");
								}
							};

							std::string temps = cptoutf8(c16);

							for (const auto& tempc : temps)
							{
								add(tempc);
							}
							unicode = false;
						}
						continue;
					}

					if (c == '=' || c == ':') // delimiting characters
					{
						// if previous character is space delimiter, these act as delimiters
						// otherwise, add them as a normal character
						if (isvalue)
						{
							if (!value.empty() || !delim_is_space)
							{
								value += c;
							}
							else
							{
								delim_is_space = false;
							}
						}
						else
						{
							isvalue = true;
						}
						blank = false;
					}
					else if (c == ' ' || c == '\t') // spaces/tabs are handled differently based on context
					{
						if (isvalue)
						{
							// if value is still empty, space acts as delimiter
							// spaces don't have special meaning when in value, just add it on
							// blank spaces at the beginning of an escaped line should still be ignored
							if (!value.empty() && !blank)
							{
								value += c;
							}
						}
						else
						{
							// ignore if there are only blank characters
							// otherwise, spaces act as delimiter
							if (!blank)
							{
								isvalue = true;
								delim_is_space = true;
							}
						}
					}
					else if (c == '#' || c == '!') // comments
					{
						// first non-blank character, skip this line
						// we use key.empty() here instead of blank because
						// blank gets reset if we escape a newline
						if (key.empty())
						{
							skipline = true;
							break;
						}
						else
						{
							add(c);
						}
						blank = false;
					}
					else if (c == '\\') // escape
					{
						escape = true;
						blank = false;
					}
					else
					{
						add(c);
						blank = false;
					}
					checkpoint();
				}
				if (!skipline && !escape && !key.empty())
				{
					m[key] = value;
				}
			}
			checkpoint();
			return m;
		}

		std::pair<int, int> parse_range(const std::string_view& s)
		{
			std::string str;
			for (const auto& c : s)
			{
				if (c != '(' && c != ')')
				{
					str += c;
				}
			}

			if (str.empty())
			{
				output<level_t::error>("Range to parse is empty");
				return { INT_MIN, INT_MIN };
			}

			size_t pos;
			if (str.starts_with('-'))
			{
				// find second - character
				pos = str.find('-', 1);
			}
			else
			{
				// find first - character
				pos = str.find('-');
			}
			const auto substr1 = str.substr(0, pos);
			const auto substr2 = str.substr(pos + 1);
			try
			{
				checkpoint();
				return { std::stoi(substr1), std::stoi(substr2) };
			}
			catch (const std::invalid_argument& e)
			{
				output<level_t::error>("{}\nstoi arguments:\"{}\", \"{}\"", e.what(), substr1, substr2);
				return { INT_MIN, INT_MIN };
			}
		}
	}

	const outstream& outstream::operator<<(const std::string_view& str) const noexcept
	{
#ifdef GUI_OUTPUT
		// output to sstream regardless of outputlevel
		if (argc < 2)
		{
			sstream << str;
		}
#else
		if (cout)
		{
			if (err)
			{
				std::cerr << str;
			}
			else
			{
				std::cout << str;
			}
		}
#endif
		if (file)
		{
#ifdef __EMSCRIPTEN__
			std::cout << str;
#else
			if (logfile.good())
			{
				logfile << str;
			}
#endif
		}
		return *this;
	}

	const outstream& outstream::operator<<(std::ostream& (*f)(std::ostream&)) const noexcept
	{
#ifdef GUI_OUTPUT
		if (argc < 2)
		{
			if (f == static_cast<std::basic_ostream<char>&(*)(std::basic_ostream<char>&)>(&std::endl))
			{
				updateoutput();
			}
			else
			{
				sstream << f;
			}
		}
#else
		if (cout)
		{
			if (err)
			{
				std::cerr << f;
			}
			else
			{
				std::cout << f;
			}
		}
#endif
		if (file)
		{
#ifdef __EMSCRIPTEN__
			std::cout << f;
#else
			if (logfile.good())
			{
				logfile << f;
			}
#endif
		}
		return *this;
	}

	bool copy(const std::filesystem::path& from, const std::filesystem::path& to) noexcept
	{
		checkpoint();
		try
		{
			if (!std::filesystem::exists(from))
			{
				output<level_t::error>("Error: tried to copy nonexistent file\n{}", c8tomb(from.generic_u8string()));
				return false;
			}
			if (std::filesystem::is_directory(to) != std::filesystem::is_directory(from))
			{
				output<level_t::error>("Error: tried to copy a file to a directory (or vice versa)\n{}\n{}", c8tomb(from.generic_u8string()), c8tomb(to.generic_u8string()));
				return false;
			}

			if (std::filesystem::exists(to))
			{
				try
				{
					std::filesystem::remove(to);
				}
				catch (const std::filesystem::filesystem_error& e)
				{
					output<level_t::error>("Error removing file:\n{}", e.what());
					return false;
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
					output<level_t::error>("Error creating directory:\n{}", e.what());
					return false;
				}
			}

			try
			{
				std::filesystem::copy(from, to);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				output<level_t::error>("Error copying file:\n{}", e.what());
				return false;
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			output<level_t::error>("Filesystem Error: {}", e.what());
			return false;
		}

		return true;
	}

	// get pack version from pack.mcmeta
	// @param path  path of resource pack (folder)
	// @return pack version as integer
	static int getpackver(const std::filesystem::path& path)
	{
		checkpoint();
		const std::filesystem::path pack_mcmeta = path / "pack.mcmeta";
		if (!std::filesystem::is_regular_file(pack_mcmeta))
		{
			output<level_t::warning>("pack.mcmeta not found; in {}", c8tomb(path.filename().u8string()));
			return -1;
		}
		nlohmann::json j;
		std::ifstream fin(pack_mcmeta, std::ios::binary);
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
			output<level_t::warning>("Json error while parsing pack.mcmeta from {}:\n{}", c8tomb(path.filename().u8string()), e.what());
		}
		fin.close();
		return -1;
	}

	// find item in zip archive
	// @param ziparchive  path to zipped resourcepack
	// @param itemtofind  item to find in zip archive (will match if starts with)
	// @return whether item is find
	static bool findzipitem(const std::filesystem::path& ziparchive, const std::u8string_view& itemtofind) noexcept
	{
		checkpoint();
		try
		{
			bool found = false;
			mz_zip_archive archive = mz_zip_archive();
			mz_zip_reader_init_file(&archive, c8tomb(ziparchive.generic_u8string().c_str()), 0);
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
			checkpoint();
			return found;
		}
		catch (const std::exception& e)
		{
			output<level_t::error>("Error: {}", e.what());
			return false;
		}
	}

	bool findfolder(const std::filesystem::path& path, const std::u8string_view& tofind, const bool zip) noexcept
	{
		checkpoint();
		if (zip)
		{
			return findzipitem(path, tofind);
		}
		else
		{
			try
			{
				return std::filesystem::exists(path / tofind);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				output<level_t::error>("Filesystem Error: {}", e.what());
				return false;
			}
		}
	}

	// unzip a zip file
	// @param path  path to extract to
	// @param zip  zip archive to extract
	static void unzip(const std::filesystem::path& path, Zippy::ZipArchive& zipa)
	{
		output<level_t::important>("Extracting {}", c8tomb(path.filename().u8string()));
		zipa.Open(c8tomb(path.generic_u8string()));
		const std::u8string folder = path.stem().generic_u8string();
		std::filesystem::create_directories(std::filesystem::path(u8"mcpppp-temp/" + folder));
		zipa.ExtractAll("mcpppp-temp/" + c8tomb(folder) + '/');
		checkpoint();
	}

	// zip a folder into a zip file
	// @param folder  folder to zip
	// @param zipa  zip archive containing zip file name
	static void rezip(const std::u8string& folder, Zippy::ZipArchive& zipa)
	{
		output<level_t::important>("Compressing {}", c8tomb(folder));
		Zippy::ZipEntryData zed;
		const size_t length = 13 + folder.size();
		for (const auto& png : std::filesystem::recursive_directory_iterator(std::filesystem::path(u8"mcpppp-temp/" + folder)))
		{
			if (png.is_directory())
			{
				continue;
			}
			std::ifstream fin(png.path(), std::ios::binary);
			zed = std::vector<unsigned char>{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
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
		checkpoint();
	}

	// items in folder from getitems()
	static std::vector<std::pair<std::filesystem::directory_entry, std::u8string>> items;

	// get all items in folder into `items`
	// @param path  folder to get items from
	// @return number of items
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
		checkpoint();
		return size;
	}

	// compute 128-bit hash of resource pack
	// @param path  path of resource pack
	// @param zip  whether resource pack is zipped
	// @return hex of 128-bit hash
	static std::string hash(const std::filesystem::path& path, const bool zip)
	{
		output<level_t::important>("Computing Hash: {}", c8tomb(path.filename().u8string()));
		if (zip)
		{
			std::ifstream fin(path, std::ios::binary);
			std::vector<char> file_contents{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
			fin.close();
			checkpoint();
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
			checkpoint();
			return hashvalue;
		}
	}

	std::unordered_map<conversions, checkresults> getconvstatus(const std::filesystem::path& path, const bool dofsb, const bool dovmt, const bool docim) noexcept
	{
		try
		{
			std::unordered_map<conversions, checkresults> status;
			const bool zip = (path.extension() == ".zip");
			const checkinfo fsb = fsb::check(path, zip), vmt = vmt::check(path, zip), cim = cim::check(path, zip);

			status[conversions::fsb] = fsb.results;
			status[conversions::vmt] = vmt.results;
			status[conversions::cim] = cim.results;

			checkpoint();
			return status;
		}
		catch (const std::exception& e)
		{
			output<level_t::error>("Error: {}", e.what());
			return {};
		}
	}

	bool convert(const std::filesystem::path& path, const bool dofsb, const bool dovmt, const bool docim, const bool force_reconvert)
	{
		if (!std::filesystem::is_directory(path) && path.extension() != ".zip")
		{
			output<level_t::error>("Tried to convert invalid pack:\n{}", c8tomb(path.generic_u8string()));
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
		const auto isvalid = [](const checkinfo info, const bool doconversion, const bool strict = false) -> bool
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
		const auto isreconvert = [](const checkinfo info, const bool doconversion) -> bool
		{
			if (!doconversion || !autoreconvert)
			{
				return false;
			}
			return info.results == checkresults::reconverting;
		};

		// only compute hash if possibly reconverting and not always reconverting
		if (force_reconvert)
		{
			reconvert = true;
		}
		else if (isreconvert(fsb, dofsb) || isreconvert(vmt, dovmt) || isreconvert(cim, docim))
		{
			// no point of calculating hash if we have nothing to compare it to
			if (hashes.contains(namehash))
			{
				hashvalue = hash(path, zip);
				if (hashes[namehash] != hashvalue)
				{
					output<level_t::info>("Pack appears to have changed: {}, reconverting", c8tomb(path.filename().u8string()));
				}
				else
				{
					// don't reconvert if pack hasn't changed
					reconvert = false;
				}
				checkpoint();
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
				checkpoint();
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
				output<level_t::info>("FSB: Nothing to convert in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::alrfound:
				output<level_t::info>("FSB: Fabricskyboxes folder found in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					output<level_t::important>("FSB: Reconverting {}", c8tomb(path.filename().u8string()));
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/fabricskyboxes"));
					fsb::convert(convert, path.filename().u8string(), fsb);
				}
				else
				{
					output<level_t::info>("FSB: Fabricskyboxes folder found in {}, skipping", c8tomb(path.filename().u8string()));
				}
				break;
			}
			checkpoint();
		}
		if (dovmt)
		{
			switch (vmt.results)
			{
			case checkresults::valid:
				vmt::convert(convert, path.filename().u8string(), vmt);
				break;
			case checkresults::noneconvertible:
				output<level_t::info>("VMT: Nothing to convert in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::alrfound:
				output<level_t::info>("VMT: Varied Mob Textures folder found in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					output<level_t::important>("VMT: Reconverting {}", c8tomb(path.filename().u8string()));
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/minecraft/varied/"));
					vmt::convert(convert, path.filename().u8string(), vmt);
				}
				else
				{
					output<level_t::info>("VMT: Varied Mob Textures folder found in {}, skipping", c8tomb(path.filename().u8string()));
				}
				break;
			}
			checkpoint();
		}
		if (docim)
		{
			switch (cim.results)
			{
			case checkresults::valid:
				cim::convert(convert, path.filename().u8string(), cim);
				break;
			case checkresults::noneconvertible:
				output<level_t::info>("CIM: Nothing to convert in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::alrfound:
				output<level_t::info>("CIM: Chime folder found in {}, skipping", c8tomb(path.filename().u8string()));
				break;
			case checkresults::reconverting:
				if (reconvert)
				{
					output<level_t::important>("CIM: Reconverting {}", c8tomb(path.filename().u8string()));
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/mcpppp"));
					std::filesystem::remove_all(std::filesystem::path(convert + u8"/assets/minecraft/overrides"));
					cim::convert(convert, path.filename().u8string(), cim);
				}
				else
				{
					output<level_t::info>("CIM: Chime folder found in {}, skipping", c8tomb(path.filename().u8string()));
				}
				break;
			}
			checkpoint();
		}

		// put it here, because we want to output the error messages if it isn't valid
		if (!isvalid(fsb, dofsb, !reconvert) && !isvalid(vmt, dovmt, !reconvert) && !isvalid(cim, docim, !reconvert))
		{
			return false;
		}

		// reading and writing to/from the zip has to be done before compression
		const int packver = getpackver(convert);
		const std::filesystem::path pack_mcmeta = std::filesystem::path{ convert } / "pack.mcmeta";
		// always have a backup of the original file
		std::filesystem::path pack_mcmeta_old = std::filesystem::path{ convert } / "pack.mcmeta.old";
		if (packver != -1)
		{
			{
				int count = 2;
				while (std::filesystem::exists(pack_mcmeta_old))
				{
					pack_mcmeta_old = std::filesystem::path{ convert } / ("pack" + std::to_string(count) + ".mcmeta.old");
					count++;
				}
			}

			nlohmann::json j;
			std::ifstream fin{ pack_mcmeta, std::ios::binary };
			fin >> j;
			std::vector<char> file_contents{ std::istreambuf_iterator<char>{fin}, {} };;
			fin.close();

			std::filesystem::copy(pack_mcmeta, pack_mcmeta_old);

			j["pack"]["pack_format"] = PACK_VER;
			std::ofstream fout{ pack_mcmeta, std::ios::binary };
			fout << j.dump(1, '\t') << std::endl;
			fout.write(file_contents.data(), file_contents.size());
			fout.close();
			checkpoint();
		}

		if (zip)
		{
			rezip(folder, zipa);
			checkpoint();
		}

		hashvalue = hash(path, zip);
		hashes[namehash] = hashvalue;
		savehashes();
		checkpoint();

		if (packver != -1)
		{
			const std::string message = fmt::format(
				"Potentially incorrect pack_format in {}. It has been automatically updated, the original file can be found at {}.\n"
				"Version found: {}\n"
				"Latest version: {}",
				c8tomb(path.filename().u8string()), c8tomb(pack_mcmeta_old.filename().u8string()), packver, PACK_VER);
			output<level_t::warning>("{}", message);
#ifdef GUI
			alerts.push_back(message);
#endif
		}

		checkpoint();
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
			std::ifstream hashfile("mcpppp-hashes.json", std::ios::binary);
			std::vector<char> contents(filesize);
			hashfile.read(contents.data(), filesize);
			hashfile.close();
			hashes = nlohmann::json::parse(contents);
		}
		checkpoint();
	}

	void savehashes()
	{
		std::ofstream hashfile("mcpppp-hashes.json");
		hashfile << hashes.dump(1, '\t') << std::endl;
		hashfile.close();
		checkpoint();
	}

	// parse single setting and set necessary variables
	// @param option  setting name
	// @param j  value of setting
	static void setting(const std::string& option, const nlohmann::json& j)
	{
		if (!settings.contains(lowercase(option)))
		{
			output<level_t::warning>("Unknown setting: {}", option);
			return;
		}
		const setting_item item = settings.at(lowercase(option));
		const type_t t = item.type;
		if (t == type_t::boolean)
		{
			try
			{
				item.get<bool>() = j.get<bool>();
			}
			catch (const nlohmann::json::exception&)
			{
				output<level_t::error>("Not a valid value for {}: {}; Expected bool", option, j.dump(-1));
			}
		}
		else if (t == type_t::integer)
		{
			try
			{
				int temp = j.get<int>();
				if (item.min != 0 && item.max != 0 && (temp < item.min || temp > item.max))
				{
					output<level_t::error>("Not a valid value for {}: {}; Expected integer between {} and {}", option, temp, item.min, item.max);
				}
				item.get<level_t>() = static_cast<level_t>(temp);
			}
			catch (const nlohmann::json::exception&)
			{
				output<level_t::error>("Not a valid value for {}: {}; Expected int", option, j.dump(-1));
			}
		}
		else
		{
			try
			{
				item.get<std::string>() = j.get<std::string>();
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
				output<level_t::error>("Not a valid value for {}: {}; Expected string", option, j.dump(-1));
			}
		}
		checkpoint();
	}

	void readconfig()
	{
		paths.clear();
		if (!config.contains("settings"))
		{
			output<level_t::warning>("No settings found");
		}
		else if (config["settings"].type() != nlohmann::json::value_t::object)
		{
			output<level_t::error>("Settings must be an object, got {}", config["settings"].type_name());
		}
		else
		{
			for (const auto& j : config["settings"].items())
			{
				setting(j.key(), j.value());
			}
			checkpoint();
		}
		if (!config.contains("paths"))
		{
			output<level_t::warning>("No paths found");
			config["paths"] = nlohmann::json::array();
		}
		else if (config["paths"].type() != nlohmann::json::value_t::array)
		{
			output<level_t::error>("Paths must be an array, got {}", config["paths"].type_name());
			throw std::invalid_argument(std::string("paths must be array, got ") + config["paths"].type_name());
		}
		else
		{
			for (const auto& it : config["paths"])
			{
				// check that path exists, otherwise canonical will fail
				if (std::filesystem::exists(mbtoc8(it.get<std::string_view>())))
				{
					paths.insert(std::filesystem::canonical(mbtoc8(it.get<std::string_view>())));
				}
				else
				{
					output<level_t::warning>("Invalid path: {}", it.dump(-1));
				}
			}
			checkpoint();
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
					checkpoint();
				}
				if (config["gui"].contains("paths") && config["gui"]["paths"].type() == nlohmann::json::value_t::array)
				{
					for (const auto& it : config["gui"]["paths"])
					{
						// check that path exists, otherwise canonical will fail
						if (std::filesystem::exists(mbtoc8(it.get<std::string_view>())))
						{
							paths.insert(std::filesystem::canonical(mbtoc8(it.get<std::string_view>())));
						}
						else
						{
							output<level_t::warning>("Invalid path: {}", it.dump(-1));
						}
					}
					checkpoint();
				}
				if (config["gui"].contains("excludepaths") && config["gui"]["excludepaths"].type() == nlohmann::json::value_t::array)
				{
					for (const std::string& path : config["gui"]["excludepaths"].get<std::vector<std::string>>())
					{
						paths.erase(mbtoc8(path));
					}
					checkpoint();
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
			.help("Outputs more information (can be used upto 3 times)")
			.action([](const auto& a) { if (outputlevel > level_t::debug) { outputlevel = static_cast<level_t>(static_cast<int>(outputlevel) - 1); } }) // hacky stuff to decrement enum
			.nargs(0)
			.default_value(false)
			.implicit_value(true)
			.append();

		parser.add_argument("--force_reconvert")
			.help("Always reconvert all resourcepacks")
			.default_value(false)
			.implicit_value(true);

		for (const auto& [key, value] : settings)
		{
			const auto getdefaultvalue = [](const setting_item& item) -> std::string
			{
				switch (item.type)
				{
				case type_t::boolean:
					return std::string(item.default_val.get<bool>() ? "true" : "false");
				case type_t::integer:
					// this shouldn't happen
					return std::string();
				case type_t::string:
					return item.default_val.get<std::string>();
				}
			};

			if (value.type == type_t::integer)
			{
				parser.add_argument("--" + std::string(value.formatted_name))
					.help(std::string(value.description))
					.default_value(value.default_val.get<int>());
			}
			else
			{
				parser.add_argument("--" + std::string(value.formatted_name))
					.help(std::string(value.description))
					.default_value(getdefaultvalue(value));
			}
		}
		checkpoint();


		parser.add_argument("resourcepacks")
			.help("The resourcepacks to convert")
			.remaining();

		try
		{
			parser.parse_args(argc, argv);
		}
		catch (const std::runtime_error& e)
		{
			output<level_t::error>("Argument parse error: {}", e.what());
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
				output<level_t::warning>("Unrecognized value (expected true, false): {}\nInterpreting as false", str);
				return false;
			}
		};

		for (const auto& [key, value] : settings)
		{
			if (parser.is_used("--" + std::string(value.formatted_name)))
			{
				switch (value.type)
				{
				case type_t::boolean:
					value.get<bool>() =
						truefalse(parser.get<std::string>("--" + std::string(value.formatted_name)));
					break;

				case type_t::integer:
					value.get<level_t>() =
						static_cast<level_t>(parser.get<int>("--" + std::string(value.formatted_name)));
					break;

				case type_t::string:
					value.get<std::string>() =
						parser.get<std::string>("--" + std::string(value.formatted_name));

					// special case
					if (value.formatted_name == "log")
					{
						logfile.open(logfilename);
					}
					break;
				}
			}
		}
		checkpoint();

		try
		{
			auto resourcepacks = parser.get<std::vector<std::string>>("resourcepacks");
			resourcepacks.erase(
				std::remove_if(resourcepacks.begin(), resourcepacks.end(), [](const std::string& s) -> bool
					{
						if (!std::filesystem::exists(s))
						{
							output<level_t::error>("Invalid pack: {}", s);
							return true;
						}
						return false;
					}), resourcepacks.end());

			std::transform(resourcepacks.begin(), resourcepacks.end(), std::back_inserter(mcpppp::entries),
				[force_reconvert = (parser["--force_reconvert"] == true)](const std::string& s)->resourcepack_entry
			{
				return { true, force_reconvert, std::filesystem::directory_entry(s) };
			});
		}
		catch (const std::logic_error& e)
		{
			output<level_t::error>("Error while parsing arguments: {}", e.what());
			std::exit(0);
		}
		checkpoint();
	}
}
