/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cctype>
#include <climits>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "reselect.h"
#include "convert.h"
#include "utility.h"

using mcpppp::out;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

// temporary thing so i can compile
#define VMT ""

namespace vmt
{
	static std::set<std::string> names;

	// separate name and number (e.g. zombie12 -> zombie, 12)
	// @param str  filename to separate
	// @return pair of string (name) and int
	static std::pair<std::string, int> separate(const std::string& str)
	{
		std::string name = str, num;
		for (size_t i = str.size() - 1; i >= 0; i--)
		{
			if (std::isdigit(str[i]) != 0)
			{
				num.push_back(str[i]);
			}
			else
			{
				name.erase(name.begin() + i + 1, name.end());
				break;
			}
		}
		std::reverse(num.begin(), num.end());
		return { name, std::stoi(num) };
	}

	// moves vmt pngs to new location
	// @param path  path of resourcepack being converted
	// @param optifine  whether to use optifine locations (no = mcpatcher)
	// @param newlocation  whether to use new optifine location
	// @param entry  actual png file to convert
	static void png(const std::u8string& path, const bool& optifine, const bool& newlocation, const std::filesystem::directory_entry& entry)
	{
		const auto p = separate(c8tomb(entry.path().filename().stem().u8string()));
		const std::string curname = p.first;
		// current name is new, look for all textures
		if (!names.contains(curname))
		{
			std::filesystem::path folderpath;
			std::vector<reselect> paths = { reselect("default", false) };

			// get directory that files are in
			const std::filesystem::path location = (optifine ? (newlocation ? u8"assets/minecraft/optifine/random/entity" : u8"assets/optifine/mob") : u8"assets/mcpatcher/mob");
			// I'd prefer to use std::filesystem::relative here, but given 2 equal paths it returns "." instead of ""
			std::u8string tempfolderpath = std::filesystem::canonical(path / location).generic_u8string();
			tempfolderpath.erase(tempfolderpath.begin(),
				tempfolderpath.begin() + std::filesystem::canonical(entry.path().parent_path()).generic_u8string().size());
			folderpath = tempfolderpath;

			// search for name1.png, name2.png, etc
			for (int curnum = 2; curnum < std::numeric_limits<int>::max(); curnum++)
			{
				std::filesystem::path texture = entry.path().parent_path() / (mbtoc8(curname + std::to_string(curnum) + ".png"));
				// doesn't exist, textures have ended
				if (!std::filesystem::exists(texture))
				{
					break;
				}

				mcpppp::copy(texture, std::filesystem::path(path + u8"/assets/mcpppp/vmt/") / folderpath / mbtoc8(curname + std::to_string(curnum) + ".png"));

				paths.push_back(c8tomb((u8"mcpppp:vmt/" / folderpath / mbtoc8(curname + std::to_string(curnum) + ".png")).generic_u8string()));
			}
			names.insert(curname);

			// randomly select between each texture, if there is more than just the default path
			if (paths.size() > 1)
			{
				reselect res;
				res.random(curname, paths);
				const std::string str = res.to_string();
				std::ofstream fout(std::filesystem::path(path) / "assets/mcpppp/vmt" / folderpath / (curname + ".reselect"));
				fout.write(str.c_str(), str.size());
				fout.close();
			}
		}
	}

	// converts optifine properties to vmt/reselect
	static void prop(const std::u8string& path, const bool& newlocation, const std::filesystem::directory_entry& entry)
	{
		long long curnum;
		std::u8string name, folderpath;
		std::vector<std::string> biomelist = { "ocean", "deep_ocean", "frozen_ocean", "deep_frozen_ocean", "cold_ocean", "deep_cold_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean", "deep_warm_ocean", "river", "frozen_river", "beach", "stone_shore", "snowy_beach", "forest", "wooded_hills", "flower_forest", "birch_forest", "birch_forest_hills", "tall_birch_forest", "tall_birch_hills", "dark_forest", "dark_forest_hills", "jungle", "jungle_hills", "modified_jungle", "jungle_edge", "modified_jungle_edge", "bamboo_jungle", "bamboo_jungle_hills", "taiga", "taiga_hills", "taiga_mountains", "snowy_taiga", "snowy_taiga_hills", "snowy_taiga_mountains", "giant_tree_taiga", "giant_tree_taiga_hills", "giant_spruce_taiga", "giant_spruce_taiga_hills", "mushroom_fields", "mushroom_field_shore", "swamp", "swamp_hills", "savanna", "savanna_plateau", "shattered_savanna", "shattered_savanna_plateau", "plains", "sunflower_plains", "desert", "desert_hills", "desert_lakes", "snowy_tundra", "snowy_mountains", "ice_spikes", "mountains", "wooded_mountains", "gravelly_mountains", "modified_gravelly_mountains", "mountain_edge", "badlands", "badlands_plateau", "modified_badlands_plateau", "wooded_badlands_plateau", "modified_wooded_badlands_plateau", "eroded_badlands", "dripstone_caves", "lush_caves", "nether_wastes", "crimson_forest", "warped_forest", "soul_sand_valley", "basalt_deltas", "the_end", "small_end_islands", "end_midlands", "end_highlands", "end_barrens", "the_void" };
		folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5)));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		name = entry.path().stem().u8string();
		std::string temp, option, value, time1, height1, tempnum;
		nlohmann::json j, tempj;
		std::vector<nlohmann::json> v, tempv;
		std::vector<std::vector<int>> weights;
		std::vector<std::vector<std::pair<std::string, std::string>>> times, heights;
		std::vector<std::vector<std::string>> biomes, textures;
		std::vector<std::array<bool, 4>> weather;
		std::vector<std::pair<std::string, signed char>> names; // -1 = normal string, 0 = regex, 1 = iregex
		std::vector<int> baby, minheight, maxheight;
		std::stringstream ss;
		std::ifstream fin(entry.path());
		while (fin)
		{
			std::getline(fin, temp);
			option.clear();
			value.clear();
			bool isvalue = false;
			if (temp.empty() || temp.front() == '#')
			{
				continue;
			}
			for (const char& c : temp)
			{
				if (c == '=')
				{
					isvalue = true;
				}
				else if (!isvalue)
				{
					option += c;
				}
				else // isvalue
				{
					value += c;
				}
			}
			tempnum.clear();
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
				names.resize(static_cast<size_t>(curnum), std::make_pair("", -1));
				weather.resize(static_cast<size_t>(curnum), { false, false, false, false });
				minheight.resize(static_cast<size_t>(curnum), INT_MIN);
				maxheight.resize(static_cast<size_t>(curnum), INT_MIN);
			}
			if (option.starts_with("textures.") || option.starts_with("skins."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						if (temp == "1")
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back(c8tomb(u8"minecraft:textures/entity/" + folderpath + name + u8".png"));
						}
						else
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back(c8tomb(u8"minecraft:varied/textures/entity/" + folderpath + name + mbtoc8(temp) + u8".png"));
						}
					}
				}
			}
			else if (option.starts_with("weights."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						weights.at(static_cast<size_t>(curnum - 1)).push_back(stoi(temp));
					}
				}
			}
			else if (option.starts_with("biomes."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						// std::string::contains in C++23
						if (temp.find(':') == std::string::npos) // does not contain a namespace
						{
							for (std::string& s : biomelist)
							{
								if (mcpppp::ununderscore(s) == mcpppp::lowercase(temp))
								{
									biomes.at(static_cast<size_t>(curnum - 1)).push_back("minecraft:" + s);
									break;
								}
							}
						}
						else // contains a namespace
						{
							biomes.at(static_cast<size_t>(curnum - 1)).push_back(temp);
						}
					}
				}
			}
			else if (option.starts_with("heights."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						height1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								break;
							}
							height1 += temp.at(i);
						}
						heights.at(static_cast<size_t>(curnum - 1)).push_back(std::make_pair(height1, temp));
					}
				}
			}
			else if (option.starts_with("minHeight"))
			{
				minheight.at(static_cast<size_t>(curnum - 1)) = stoi(value);
			}
			else if (option.starts_with("maxHeight"))
			{
				maxheight.at(static_cast<size_t>(curnum - 1)) = stoi(value);
			}
			else if (option.starts_with("name."))
			{
				temp = value;
				bool insensitive = false;
				if (temp.starts_with("regex:") || temp.starts_with("iregex:"))
				{
					// if first character is i, then it is iregex (case insensitive)
					insensitive = (temp.front() == 'i');
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
				}
				else if (temp.starts_with("pattern:") || temp.starts_with("ipattern:"))
				{
					insensitive = (temp.front() == 'i');
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = mcpppp::oftoregex(temp);
				}
				names.at(static_cast<size_t>(curnum - 1)) = std::make_pair(temp, insensitive);
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
			}
			else if (option.starts_with("health."))
			{
				// TODO
			}
			else if (option.starts_with("moonPhase."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("dayTime."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						time1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								break;
							}
							time1 += temp.at(i);
						}
						times.at(static_cast<size_t>(curnum - 1)).push_back(std::make_pair(time1, temp));
					}
				}
			}
			else if (option.starts_with("weather."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (temp == "clear")
					{
						weather.at(static_cast<size_t>(curnum)).at(1) = true;
						weather.at(static_cast<size_t>(curnum)).at(0) = true;
					}
					if (temp == "rain")
					{
						weather.at(static_cast<size_t>(curnum)).at(2) = true;
						weather.at(static_cast<size_t>(curnum)).at(0) = true;
					}
					if (temp == "thunder")
					{
						weather.at(static_cast<size_t>(curnum)).at(3) = true;
						weather.at(static_cast<size_t>(curnum)).at(0) = true;
					}
				}
			}
		}
		for (size_t i = 0; i < textures.size(); i++) // TODO: there's probably a better way to do this
		{
			if (textures.at(i).empty())
			{
				continue;
			}
			if (!weights.at(i).empty())
			{
				int weightsum = 0;
				tempv.clear();
				for (size_t k = 0; k < textures.at(i).size(); k++)
				{
					weightsum += weights.at(i).at(k);
					tempv.push_back({ {"below", weightsum}, {"then", {{"type", std::string{VMT} + ":constant"}, {"identifier", textures.at(i).at(k)}}} });
				}
				tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", weightsum}}}, {"options", tempv} };
			}
			else
			{
				tempv.clear();
				for (size_t k = 0; k < textures.at(i).size(); k++)
				{
					tempv.push_back({ {"below", k + 1}, {"then", {{"type", std::string{VMT} + ":constant"}, {"identifier", textures.at(i).at(k)}}} });
				}
				tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", textures.at(i).size()}}}, {"options", tempv} };
			}
			if (!biomes.at(i).empty())
			{
				tempj = { {"type", std::string{VMT} + ":string"}, {"when", {{"type", std::string{VMT} + ":entity_biome"}}}, {"options", {{{"match", biomes.at(i)}, {"then", tempj}}}} };
			}
			if (baby.at(i) != -1)
			{
				// TODO: no baby prop??
				/*if (baby[i])
				{
					tempj = { {"type", "varied-mobs:baby"}, {"value", tempj} };
				}
				else
				{
					tempj = { {"type", "varied-mobs:not"}, {"value", {{"type", "varied-mobs:seq"}, {"choices", {nlohmann::json({{"type", "varied-mobs:baby"}, {"value", ""}}), tempj}}}} };
				}*/
			}
			if (!times.at(i).empty())
			{
				// TODO: no daytime prop?
				/*for (int j = 0; j < times[i].size(); j++)
				{
					tempv.push_back({ {"type", "varied-mobs:time-prop"}, {"positions", nlohmann::json::array({times[i][j].first, times[i][j].second})}, {"choices", {tempj}} });
				}
				tempj = { {"type", "varied-mobs:seq"}, {"choices", tempv} };*/
			}
			if (!heights.at(i).empty())
			{
				tempv.clear();
				std::transform(heights.at(i).begin(), heights.at(i).end(), std::back_inserter(tempv), [&tempj](const std::pair<std::string, std::string>& p) -> nlohmann::json
					{
						return nlohmann::json({ {"above", p.first}, {"below", p.second}, {"then", tempj} });
					});
				tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":entity_y"}}}, {"options", tempv} };
			}
			if (minheight.at(i) != INT_MIN)
			{
				if (maxheight.at(i) == INT_MIN)
				{
					tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":entity_y"}}}, {"options", { {"above", minheight.at(i)}, {"then", tempj} }} };
				}
				else
				{
					tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":entity_y"}}}, {"options", { {"above", minheight.at(i)}, {"below", maxheight.at(i)}, {"then", tempj} }} };
				}
			}
			else if (maxheight.at(i) != INT_MIN)
			{
				tempj = { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":entity_y"}}}, {"options", {{"below", maxheight.at(i)}, {"then", tempj} }} };
			}
			if (weather.at(i).at(0))
			{
				// TODO: no weather prop?
				/*tempj = {{"type", "varied-mobs:weather-prop"}, {"positions", {0, 1, 2, 2}}, {"choices", {weather[i][1] ? tempj : nlohmann::json(), weather[i][2] ? tempj : nlohmann::json(), weather[i][3] ? tempj : nlohmann::json()}}};*/
			}
			if (!names.at(i).first.empty())
			{
				std::string type;
				switch (names.at(i).second)
				{
				case -1:
					type = "match"; // normal string
					break;
				case 0:
					type = "matchPattern"; // regex
					break;
				case 1:
					type = "iMatchPattern"; // iregex
					break;
				default:
					type = "match";
				}
				tempj = { {"type", std::string{VMT} + ":string"}, {"use", {{"type", std::string{VMT} + ":entity_name"}}}, {"options", {{{type, names.at(i).first}, {"then", tempj}}}} };
			}
			v.push_back(tempj);
		}
		tempv.clear();
		for (size_t i = 0; i < v.size(); i++)
		{
			tempv.push_back({ {"below", i + 1}, {"then", v.at(i)} });
		}
		j = { {"version", 1}, {"root", {{"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", tempv}}} };
		std::filesystem::create_directories(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath);
		std::ofstream fout(std::filesystem::path(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath + name + u8".json"));
		fout << j.dump(1, '\t') << std::endl;
	}

	// check if should be converted
	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip)
	{
		using mcpppp::checkresults;
		bool reconverting = false;
		if (mcpppp::findfolder(path.u8string(), u8"assets/mcpppp/vmt/", zip))
		{
			if (mcpppp::autoreconvert)
			{
				reconverting = true;
			}
			else
			{
				return { checkresults::alrfound, false, false };
			}
		}
		if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/optifine/random/entity/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, true };
			}
			else
			{
				return { checkresults::valid, true, true };
			}
		}
		else if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/optifine/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, false };
			}
			else
			{
				return { checkresults::valid, true, false };
			}
		}
		else if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/mcpatcher/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, false, false };
			}
			else
			{
				return { checkresults::valid, false, false };
			}
		}
		else
		{
			return { checkresults::noneconvertible, false, false };
		}
	}

	// main vmt function
	void convert(const std::u8string& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
		// source: assets/minecraft/optifine/random/entity/
		// destination: assets/mcpppp/vmt/

		out(3) << "VMT: Converting Pack " << c8tomb(filename) << std::endl;

		// convert all images first, so the reselect file can be overridden
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			std::filesystem::path(path + u8"/assets/minecraft/" +
				(info.optifine ? u8"optifine" + std::u8string(info.newlocation ? u8"/random/entity/" : u8"/mob/") : u8"mcpatcher/mob/"))))
		{
			if (entry.path().extension() == ".png")
			{
				out(1) << "VMT: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			}
			if (entry.path().filename().extension() == ".png")
			{
				png(path, info.optifine, info.newlocation, entry);
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			std::filesystem::path(path + u8"/assets/minecraft/" +
				(info.optifine ? u8"optifine" + std::u8string(info.newlocation ? u8"/random/entity/" : u8"/mob/") : u8"mcpatcher/mob/"))))
		{
			if (entry.path().extension() == ".properties")
			{
				out(1) << "VMT: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			}
			if (entry.path().filename().extension() == ".properties")
			{
				prop(path, info.newlocation, entry);
			}
		}
	}
};
