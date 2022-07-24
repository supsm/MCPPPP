/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pch.h"

#include "constants.h"
#include "convert.h"
#include "utility.h"
#include "reselect.h"

using mcpppp::output;
using mcpppp::level_t;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;
using mcpppp::checkpoint;

namespace vmt
{
	// names of png files (axolotl69.png -> "axolotl")
	static std::unordered_set<std::string> png_names;
	// filenames of png files (relative to vmt folder)
	static std::unordered_set<std::u8string> png_filenames;

	// special mob info
	class s_mob_t
	{
	public:
		std::string type, reselect_func;
		reselect data;
	};
	static std::map<std::string, std::vector<s_mob_t>> s_mobs; // special mobs, but that name is already used

	// separate name and number (e.g. zombie12 -> zombie, 12)
	// @param str  filename to separate
	// @return pair of string (name) and int
	static std::pair<std::string, int> separate(const std::string& str)
	{
		std::string name = str, num;
		for (size_t i = str.size(); i > 0; i--)
		{
			if (std::isdigit(str[i - 1]) != 0)
			{
				num.push_back(str[i - 1]);
			}
			else
			{
				name.erase(name.begin() + i, name.end());
				break;
			}
		}
		if (num.empty())
		{
			checkpoint();
			return { name, -1 };
		}
		else
		{
			std::reverse(num.begin(), num.end());
			checkpoint();
			return { name, std::stoi(num) };
		}
	}

	// moves vmt pngs to new location
	// @param path  path of resourcepack being converted
	// @param optifine  whether to use optifine locations (no = mcpatcher)
	// @param newlocation  whether to use new optifine location
	// @param entry  actual png file to convert
	static void png(const std::filesystem::path& path, const bool optifine, const bool newlocation, const bool zip, const std::filesystem::directory_entry& entry)
	{
		const std::u8string mcnamespace = u8"mcpppp_" + mbtoc8(mcpppp::conv::getfilenamehash(path, zip));
		const auto p = separate(c8tomb(entry.path().filename().stem().generic_u8string()));
		const std::string curname = p.first;
		// current name is new, look for all textures
		if (p.second != -1 && !png_names.contains(curname))
		{
			std::filesystem::path folderpath;
			std::vector<reselect> paths = { reselect("default", false) };

			{
				// get directory that files are in
				const std::filesystem::path location = (optifine ? (newlocation ? u8"assets/minecraft/optifine/random/entity" : u8"assets/minecraft/optifine/mob") : u8"assets/minecraft/mcpatcher/mob");
				// I'd prefer to use std::filesystem::relative here, but given 2 equal paths it returns "." instead of ""
				std::u8string tempfolderpath = std::filesystem::canonical(entry.path().parent_path()).generic_u8string();
				tempfolderpath.erase(tempfolderpath.begin(),
					tempfolderpath.begin() + std::filesystem::canonical(path / location).generic_u8string().size());
				if (!tempfolderpath.empty())
				{
					tempfolderpath.erase(tempfolderpath.begin());
				}
				folderpath = tempfolderpath;
			}

			// search for name1.png, name2.png, etc
			for (int curnum = 2; curnum < std::numeric_limits<int>::max(); curnum++)
			{
				const std::u8string curfilename = mbtoc8(curname + std::to_string(curnum) + ".png");
				const std::filesystem::path texture = entry.path().parent_path() / curfilename;
				// doesn't exist, textures have ended
				if (!std::filesystem::exists(texture))
				{
					break;
				}

				mcpppp::copy(texture, path / u8"assets" / mcnamespace / u8"vmt" / folderpath / curfilename);

				paths.push_back(static_cast<reselect>(c8tomb(((mcnamespace + u8":vmt") / folderpath / curfilename).generic_u8string())));

				png_filenames.insert((folderpath / curfilename).generic_u8string());
			}
			png_names.insert(curname);
			checkpoint();

			// randomly select between each texture, if there is more than just the default path
			if (paths.size() > 1)
			{
				reselect res;
				res.add_random(curname, paths);
				const std::string str = res.get_string();
				std::ofstream fout(path / "assets/vmt" / (curname + ".reselect"));
				fout.write(str.c_str(), str.size());
				fout.close();
			}
			checkpoint();
		}
		else
		{
			std::filesystem::path folderpath;
			{
				const std::filesystem::path location = (optifine ? (newlocation ? u8"assets/minecraft/optifine/random/entity" : u8"assets/minecraft/optifine/mob") : u8"assets/minecraft/mcpatcher/mob");
				std::u8string tempfolderpath = std::filesystem::canonical(entry.path().parent_path()).generic_u8string();
				tempfolderpath.erase(tempfolderpath.begin(),
					tempfolderpath.begin() + std::filesystem::canonical(path / location).generic_u8string().size());
				if (!tempfolderpath.empty())
				{
					tempfolderpath.erase(tempfolderpath.begin());
				}
				folderpath = tempfolderpath;
			}

			if (!png_filenames.contains((folderpath / entry.path().filename()).generic_u8string()))
			{
				mcpppp::copy(entry.path(), path / u8"assets" / mcnamespace / u8"vmt" / folderpath / entry.path().filename().u8string());
				png_filenames.insert((folderpath / entry.path().filename()).generic_u8string());
			}
			checkpoint();
		}
	}

	// string match type
	enum class match_type { normal, regex, iregex };

	// read and parse optifine properties file
	// @param path  path to resource pack
	// @param newlocation  whether the new location (random/entity) is used instead of old location (mob)
	// @param zip  whether resource pack is zipped
	// @param entry  directory entry of properties file to parse
	// @param name  filename of properties file without extension (output)
	// @param folderpath  relative path of parent folder of `entry` from vmt folder (output)
	// @param textures  textures to select from (output)
	// @param weights  weights to apply on random selection for textures (output)
	// @param biomes  biome predicates (output)
	// @param heights  height predicates as a min and max value (output)
	// @param minheight  minheight predicate for legacy compatibility (output)
	// @param maxheight  maxheight predicate for legacy compatibility (output)
	// @param names  name predicates (output)
	// @param baby  baby predicate (output)
	// @param healths  health predicates as a tuple of <min, max, ispercent> (output)
	// @param times  day time predicates (output)
	// @param weather  [FORMAT SHOULD BE UPDATED] weather predicate (output)
	static void read_prop(const std::filesystem::path& path, const bool newlocation, const bool zip, const std::filesystem::directory_entry& entry,
		std::string& name,
		std::u8string& folderpath,
		std::vector<std::vector<std::string>>& textures,
		std::vector<std::vector<int>>& weights,
		std::vector<std::vector<std::string>>& biomes,
		std::vector<std::vector<std::pair<int, int>>>& heights,
		std::vector<std::string>& minheight,
		std::vector<std::string>& maxheight,
		std::vector<std::pair<std::string, match_type>>& names,
		std::vector<int>& baby,
		std::vector<std::vector<std::tuple<std::string, std::string, bool>>>& healths,
		std::vector<std::vector<std::pair<int, int>>>& times,
		std::vector<std::array<bool, 4>>& weather)
	{
		long long curnum = 0;
		// TODO: there's probably a better way to do this (folderpath)
		folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5)));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		name = c8tomb(entry.path().stem().generic_u8string());

		std::ifstream fin(entry.path());
		std::string rawdata{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
		const auto prop_data = mcpppp::conv::parse_properties(rawdata);
		checkpoint();

		for (const auto& [option, value] : prop_data)
		{
			std::string tempnum;
			for (size_t i = option.size(); i >= 1; i--)
			{
				if (option.at(i - 1) >= '0' && option.at(i - 1) <= '9')
				{
					tempnum.insert(tempnum.begin(), option.at(i - 1));
				}
				else
				{
					break;
				}
			}
			if (tempnum.empty())
			{
				continue;
			}
			curnum = stoi(tempnum);
			if (static_cast<size_t>(curnum) > textures.size())
			{
				textures.resize(static_cast<size_t>(curnum));
				weights.resize(static_cast<size_t>(curnum));
				biomes.resize(static_cast<size_t>(curnum));
				times.resize(static_cast<size_t>(curnum));
				baby.resize(static_cast<size_t>(curnum), -1);
				heights.resize(static_cast<size_t>(curnum));
				names.resize(static_cast<size_t>(curnum), std::make_pair("", match_type::normal));
				weather.resize(static_cast<size_t>(curnum), { false, false, false, false });
				minheight.resize(static_cast<size_t>(curnum));
				maxheight.resize(static_cast<size_t>(curnum));
				healths.resize(static_cast<size_t>(curnum));
			}
			if (option.starts_with("textures.") || option.starts_with("skins."))
			{
				// clear if already has elements (should not happen!)
				if (!textures.at(static_cast<size_t>(curnum - 1)).empty())
				{
					if (option.starts_with('t')) // starts with textures
					{
						output<level_t::info>("(warn) VMT: Duplicate predicate textures.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					}
					else
					{
						output<level_t::info>("(warn) VMT: Duplicate predicate skins.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					}
					textures.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						if (temp == "1")
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back(std::string());
						}
						else
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back("mcpppp_" + mcpppp::conv::getfilenamehash(path, zip) + ':' + c8tomb((std::filesystem::path(u8"vmt") / folderpath / mbtoc8(name) / (mbtoc8(temp) + u8".png")).generic_u8string()));
						}
					}
				}
				checkpoint();
			}
			else if (option.starts_with("weights."))
			{
				// clear if already has elements (should not happen!)
				if (!weights.at(static_cast<size_t>(curnum - 1)).empty())
				{
					output<level_t::info>("(warn) VMT: Duplicate predicate weights.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					weights.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						try
						{
							weights.at(static_cast<size_t>(curnum - 1)).push_back(stoi(temp));
						}
						catch (const std::invalid_argument& e)
						{
							output<level_t::error>("VMT Error: {}\n\tIn file \"{}\"\n\tstoi argument is \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), value);
							return;
						}
					}
				}
				checkpoint();
			}
			else if (option.starts_with("biomes."))
			{
				// clear if already has elements (should not happen!)
				if (!biomes.at(static_cast<size_t>(curnum - 1)).empty())
				{
					output<level_t::info>("(warn) VMT: Duplicate predicate biomes.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					biomes.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						// std::string::contains in C++23
						if (temp.find(':') == std::string::npos) // does not contain a namespace
						{
							// binary search for biome name
							const auto it = std::lower_bound(biomelist.begin(), biomelist.end(), temp, [](const std::string& a, const std::string& b) -> bool
								{
									return mcpppp::lowercase(mcpppp::conv::ununderscore(a)) < mcpppp::lowercase(mcpppp::conv::ununderscore(b));
								});
							if (it == biomelist.end() || (mcpppp::conv::ununderscore(*it) != mcpppp::lowercase(mcpppp::conv::ununderscore(temp))))
							{
								output<level_t::info>("(warn) VMT: Invalid biome name: {}", temp);
							}
							else
							{
								biomes.at(static_cast<size_t>(curnum - 1)).push_back("minecraft:" + *it);
							}
						}
						else // contains a namespace
						{
							biomes.at(static_cast<size_t>(curnum - 1)).push_back(temp);
						}
					}
				}
				checkpoint();
			}
			else if (option.starts_with("heights."))
			{
				// clear if already has elements (should not happen!)
				if (!heights.at(static_cast<size_t>(curnum - 1)).empty())
				{
					output<level_t::info>("(warn) VMT: Duplicate predicate heights.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					heights.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						heights.at(static_cast<size_t>(curnum - 1)).push_back(mcpppp::conv::parse_range(temp));
					}
				}
				checkpoint();
			}
			else if (option.starts_with("minHeight"))
			{
				minheight.at(static_cast<size_t>(curnum - 1)) = value;
				checkpoint();
			}
			else if (option.starts_with("maxHeight"))
			{
				maxheight.at(static_cast<size_t>(curnum - 1)) = value;
				checkpoint();
			}
			else if (option.starts_with("name."))
			{
				std::string temp = value;
				match_type type = match_type::normal;
				if (temp.starts_with("regex:") || temp.starts_with("iregex:") || temp.starts_with("pattern:") || temp.starts_with("ipattern:"))
				{
					// if first character is i, then it is iregex (case insensitive)
					type = (temp.front() == 'i' ? match_type::iregex : match_type::regex);
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					if (temp.starts_with("pattern:") || temp.starts_with("ipattern:"))
					{
						temp = mcpppp::conv::oftoregex(temp);
					}
				}
				names.at(static_cast<size_t>(curnum - 1)) = std::make_pair(temp, type);
				checkpoint();
			}
			else if (option.starts_with("professions."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("collarColors."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("baby."))
			{
				if (value == "true")
				{
					baby.at(static_cast<size_t>(curnum - 1)) = 1;
				}
				else if (value == "false")
				{
					baby.at(static_cast<size_t>(curnum - 1)) = 0;
				}
				checkpoint();
			}
			else if (option.starts_with("health."))
			{
				// clear if already has elements (should not happen!)
				if (!healths.at(static_cast<size_t>(curnum - 1)).empty())
				{
					output<level_t::info>("(warn) VMT: Duplicate predicate healths.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					healths.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						std::string health1;
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								break;
							}
							health1 += temp.at(i);
						}
						if (temp.back() == '%')
						{
							// remove percent
							temp.pop_back();
							healths.at(static_cast<size_t>(curnum - 1)).emplace_back(health1, temp, true);
						}
						else
						{
							healths.at(static_cast<size_t>(curnum - 1)).emplace_back(health1, temp, false);
						}
					}
				}
				checkpoint();
			}
			else if (option.starts_with("moonPhase."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("dayTime."))
			{
				// clear if already has elements (should not happen!)
				if (!times.at(static_cast<size_t>(curnum - 1)).empty())
				{
					output<level_t::info>("(warn) VMT: Duplicate predicate dayTime.{} in {}", curnum, c8tomb(entry.path().generic_u8string()));
					times.at(static_cast<size_t>(curnum - 1)).clear();
				}
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (!temp.empty())
					{
						times.at(static_cast<size_t>(curnum - 1)).push_back(mcpppp::conv::parse_range(temp));
					}
				}
				checkpoint();
			}
			else if (option.starts_with("weather."))
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp == "clear")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(1) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
					if (temp == "rain")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(2) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
					if (temp == "thunder")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(3) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
				}
				checkpoint();
			}
		}
	}

	// converts optifine properties to vmt/reselect
	// @param path  path to resourcepack
	// @param newlocation  use new optifine location (random/entity) instead of old location (mob)
	// @param zip  whether resourcepack is zipped
	// @param entry  directory entry of properties file
	static void prop(const std::filesystem::path& path, const bool newlocation, const bool zip, const std::filesystem::directory_entry& entry)
	{
		std::string name;
		std::u8string folderpath;
		std::vector<std::vector<std::string>> textures;
		std::vector<std::vector<int>> weights;
		std::vector<std::vector<std::string>> biomes;
		std::vector<std::vector<std::pair<int, int>>> heights;
		std::vector<std::string> minheight, maxheight;
		std::vector<std::pair<std::string, match_type>> names;
		std::vector<int> baby;
		std::vector<std::vector<std::tuple<std::string, std::string, bool>>> healths;
		std::vector<std::vector<std::pair<int, int>>> times;
		std::vector<std::array<bool, 4>> weather;
		read_prop(path, newlocation, zip, entry, name, folderpath, textures, weights, biomes, heights, minheight, maxheight, names, baby, healths, times, weather);
		if (!folderpath.empty())
		{
			folderpath.pop_back(); // remove trailing /
		}

		// check that mob is valid
		int sm_ind = -1;
		std::string raw_type;
		if (!std::binary_search(normal_mobs.begin(), normal_mobs.end(), name))
		{
			for (int i = 0; i < special_mobs.size(); i++)
			{
				if (special_mobs.at(i) == std::make_pair(name, c8tomb(folderpath)))
				{
					// if matches, set index and change name
					sm_ind = i;
					const auto p = special_mobs.at(i).split(name);
					name = p.first;
					raw_type = p.second;
					break;
				}
			}
			if (sm_ind == -1)
			{
				output<level_t::info>("(warn) VMT: Invalid/unsupported mob: {}", name);
				return;
			}
			checkpoint();
		}

		const auto formatdecimal = [&name](std::string& s) -> void
		{
			// make sure double is valid
			try
			{
				std::stod(s);
			}
			catch (const std::invalid_argument& e)
			{
				output<level_t::error>("VMT Error: {} in {}\nstod argument: {}", e.what(), name, s);
				s = "-6969.42"; // error value, may be useful in identifying what error happened
				return;
			}

			// make it a decimal by adding .0 if necessary
			// std::string::contains in C++23
			if (s.find('.') == std::string::npos)
			{
				s += ".0";
			}
		};

		reselect res, defaultstatement = reselect("default", false);
		std::vector<reselect> conditions;
		std::vector<reselect> statements;
		for (int i = 0; i < textures.size(); i++)
		{
			if (textures.at(i).empty())
			{
				output<level_t::info>("(warn) VMT: textures.{} is empty in {}", i + 1, c8tomb(entry.path().generic_u8string()));
				continue;
			}
			reselect temp_res;
			std::vector<reselect> temp_conditions;
			if (textures.at(i).size() == 1)
			{
				// don't need to select randomly if there is only 1
				if (textures.at(i).front().empty())
				{
					// empty texture = default
					temp_res.push_back("default");
				}
				else
				{
					temp_res.push_back('\"' + textures.at(i).front() + '\"');
				}
			}
			else
			{
				temp_res.add_random(name, textures.at(i), weights.at(i));
			}

			// order is important! faster things should be evaluated first, to take advantage of short-circuit evaluation
			// as of 2/9/22, the cost is "regex >>> string comparison > anything else that operates on strings > everything else"
			if (baby.at(i) != -1)
			{
				if (static_cast<bool>(baby.at(i)))
				{
					temp_conditions.emplace_back(name + ".is_baby", false);
				}
				else
				{
					temp_conditions.emplace_back("not " + name + ".is_baby", false);
				}
				checkpoint();
			}

			if (!heights.at(i).empty())
			{
				std::vector<std::string> tempv;
				std::transform(heights.at(i).begin(), heights.at(i).end(), std::back_inserter(tempv), [&name, &formatdecimal](std::pair<int, int> p) -> std::string
					{
						return fmt::format("({0}.y >= {1}.0 and {0}.y <= {2}.0)", name, p.first, p.second);
					});
				temp_conditions.push_back(reselect::construct_or(tempv));
				checkpoint();
			}
			// heights.n overrides minHeight, maxHeight
			// valid if either minheight or maxheight is set
			else if (!minheight.at(i).empty() || !maxheight.at(i).empty())
			{
				std::string s, cur_minheight = minheight.at(i), cur_maxheight = maxheight.at(i);
				formatdecimal(cur_minheight);
				formatdecimal(cur_maxheight);
				if (!minheight.at(i).empty())
				{
					s += fmt::format("{}.y >= {}", name, cur_minheight);
				}
				if (!maxheight.at(i).empty())
				{
					if (!s.empty())
					{
						s += " and ";
					}
					s += fmt::format("{}.y <= {}", name, cur_maxheight);
				}
				temp_conditions.emplace_back(s, false);
				checkpoint();
			}

			if (!healths.at(i).empty())
			{
				std::vector<std::string> tempv;
				std::transform(healths.at(i).begin(), healths.at(i).end(), std::back_inserter(tempv), [&name, &formatdecimal](const std::tuple<std::string, std::string, bool>& t) -> std::string
					{
						// for readability
						std::string lower = std::get<0>(t), upper = std::get<1>(t);
						const bool percent = std::get<2>(t);

						formatdecimal(lower);
						formatdecimal(upper);
						if (percent) // if percent
						{
							// TODO: replace with actual max health once it is implemented
							std::string maxhealth = std::to_string(mob_healths.at(name));
							formatdecimal(maxhealth);
							return fmt::format("({0}.health * 100.0 / {1} >= {2} and {0}.health * 100.0 / {1} <= {3})",
								name, maxhealth, lower, upper);
						}
						else // if value
						{
							return fmt::format("({0}.health >= {1} and {0}.health <= {2})",
								name, lower, upper);
						}
					});
				temp_conditions.push_back(reselect::construct_or(tempv));
				checkpoint();
			}

			if (!biomes.at(i).empty())
			{
				std::vector<std::string> tempv;
				std::transform(biomes.at(i).begin(), biomes.at(i).end(), std::back_inserter(tempv), [&name](const std::string& s) -> std::string
					{
						return fmt::format("{}.current_biome == \"{}\"", name, s);
					});
				temp_conditions.push_back(reselect::construct_or(tempv));
				checkpoint();
			}

			if (!names.at(i).first.empty())
			{
				std::string condition = name + ".has_name and ";
				switch (names.at(i).second)
				{
				case match_type::normal:
					condition += name + ".name == \"" + names.at(i).first + "\"";
					break;
				case match_type::regex:
					condition += name + ".name.matches(\"" + names.at(i).first + "\")";
					break;
				case match_type::iregex:
					condition += name + ".name.lowercase.matches(\"" + mcpppp::lowercase(names.at(i).first) + "\")";
					break;
				}
				temp_conditions.emplace_back(condition, false);
				checkpoint();
			}


			if (temp_conditions.empty())
			{
				defaultstatement = temp_res;
				break;
			}
			else
			{
				conditions.push_back(reselect::construct_and(temp_conditions));
			}
			statements.push_back(temp_res);
			checkpoint();
		}

		res.add_if(conditions, statements, defaultstatement);

		if (res.empty())
		{
			output<level_t::info>("(warn) VMT: No predicates found in {}", c8tomb(entry.path().generic_u8string()));
			return;
		}

		if (sm_ind == -1) // if normal supported mob
		{
			std::filesystem::create_directories(path / u8"assets/vmt/");
			std::ofstream fout(path / u8"assets/vmt" / (mbtoc8(name) + u8".reselect"));
			fout << res.get_string() << std::endl;
			fout.close();
		}
		else
		{
			const special_mob s = special_mobs.at(sm_ind);
			const std::string type = s.get_typename(raw_type);
			s_mobs[name].push_back({ type, s.reselect_func, res });
		}
		checkpoint();
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool zip) noexcept
	{
		using mcpppp::checkresults;

		bool reconverting = false;

		if (mcpppp::findfolder(path, u8"assets/vmt/", zip))
		{
			if (mcpppp::autoreconvert)
			{
				reconverting = true;
			}
			else
			{
				return { checkresults::alrfound, false, false, zip };
			}
		}
		if (mcpppp::findfolder(path, u8"assets/minecraft/optifine/random/entity/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, true, zip };
			}
			else
			{
				return { checkresults::valid, true, true, zip };
			}
		}
		else if (mcpppp::findfolder(path, u8"assets/minecraft/optifine/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, false, zip };
			}
			else
			{
				return { checkresults::valid, true, false, zip };
			}
		}
		else if (mcpppp::findfolder(path, u8"assets/minecraft/mcpatcher/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, false, false, zip };
			}
			else
			{
				return { checkresults::valid, false, false, zip };
			}
		}
		else
		{
			return { checkresults::noneconvertible, false, false, zip };
		}
		checkpoint();
	}

	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
		// source: assets/minecraft/optifine/random/entity/
		// destination: assets/mcpppp_[hash]/vmt/

		output<level_t::important>("VMT: Converting Pack {}", c8tomb(filename));

		std::vector<std::filesystem::directory_entry> pngfiles, propfiles;

		// save png files and prop files so we don't need to iterate over the directory twice
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			path / u8"assets/minecraft" /
			(info.optifine ?
				u8"optifine" / std::filesystem::path(info.vmt_newlocation ? u8"random/entity" : u8"mob") :
				u8"mcpatcher/mob")))
		{
			if (entry.path().extension() == ".png")
			{
				pngfiles.push_back(entry);
			}
			else if (entry.path().extension() == ".properties")
			{
				propfiles.push_back(entry);
			}
		}

		// convert all images first, so the reselect file can be overridden
		for (const auto& entry : pngfiles)
		{
			output<level_t::detail>("VMT: Converting {}", c8tomb(entry.path().generic_u8string()));
			png(path, info.optifine, info.vmt_newlocation, info.iszip, entry);
		}
		checkpoint();

		for (const auto& entry : propfiles)
		{
			output<level_t::detail>("VMT: Converting {}", c8tomb(entry.path().generic_u8string()));
			s_mobs.clear();
			prop(path, info.vmt_newlocation, info.iszip, entry);

			// output special mob reselect files
			for (const auto& p : s_mobs)
			{
				reselect res;
				std::vector<std::string> conditions;
				std::vector<reselect> statements;
				for (const auto& m : p.second)
				{
					conditions.push_back(p.first + '.' + m.reselect_func + " == \"" + m.type + '\"');
					statements.push_back(m.data);
				}

				res.add_if(conditions, statements);

				std::filesystem::create_directories(path / u8"assets/vmt");
				std::ofstream fout(std::filesystem::path(path / u8"assets/vmt" / (mbtoc8(p.first) + u8".reselect")));
				fout << res.get_string() << std::endl;
				fout.close();
			}
		}
		checkpoint();
	}
}
