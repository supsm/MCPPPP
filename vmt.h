/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <climits>
#include <filesystem>
#include <string>
#include <vector>

#include "utility.h"


class vmt
{
private:
	static constexpr auto VMT = "reselect";

	// moves vmt pngs to new location
	static void vmtpng(std::u8string& name, const std::u8string& path, const bool& newlocation, std::vector<int>& numbers, const std::filesystem::directory_entry& png)
	{
		std::u8string folderpath, curname, curnum;
		while (true)
		{
			curnum.clear();
			for (size_t i = png.path().filename().u8string().size() - 4; i >= 1; i--)
			{
				if (png.path().filename().u8string().at(i - 1) >= '0' && png.path().filename().u8string().at(i - 1) <= '9')
				{
					curnum.insert(curnum.begin(), png.path().filename().u8string().at(i - 1));
				}
				else
				{
					if (numbers.size() == 0)
					{
						name = png.path().filename().u8string();
						name.erase(name.begin() + static_cast<std::string::difference_type>(i), name.end());
						folderpath = png.path().generic_u8string();
						folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5));
						folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(png.path().filename().u8string().size()), folderpath.end());
					}
					curname = png.path().filename().u8string();
					curname.erase(curname.begin() + static_cast<std::string::difference_type>(i), curname.end());
					break;
				}
			}
			if (curname == name && !curnum.empty())
			{
				folderpath = png.path().generic_u8string();
				folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5));
				folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(png.path().filename().u8string().size()), folderpath.end());
				numbers.push_back(stoi(c8tomb(curnum)));
				supsm::copy(png.path(), path + u8"/assets/minecraft/varied/textures/entity/" + folderpath + png.path().filename().u8string());
				return;
			}
			if (numbers.empty())
			{
				return;
			}
			// TODO: what am i even doing
			std::vector<nlohmann::json> v;
			for (size_t i = 0; i < numbers.size(); i++)
			{
				v.push_back({ {"below", i + 1}, {"then", {{"type", std::string{VMT} + ":constant"}, {"identifier", "minecraft:varied/textures/entity/" + c8tomb(folderpath + name) + std::to_string(numbers.at(i)) + ".png"}}} });
			}
			nlohmann::json j = { {"version", 1}, {"root", {{"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", numbers.size() + 1}}}, {"options", v} } }}} };
			if (!std::filesystem::exists(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath + name + u8".json"))
			{
				std::filesystem::create_directories(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath);
				std::ofstream fout(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath + name + u8".json");
				fout << j.dump(1, '\t') << std::endl;
				fout.close();
			}
			numbers.clear();
		}
	}

	// converts optifine properties to vmt properties json
	static void vmtprop(const std::u8string& path, const bool& newlocation, const std::filesystem::directory_entry& png)
	{
		long long curnum;
		std::u8string name, folderpath;
		std::vector<std::string> biomelist = { "ocean", "deep_ocean", "frozen_ocean", "deep_frozen_ocean", "cold_ocean", "deep_cold_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean", "deep_warm_ocean", "river", "frozen_river", "beach", "stone_shore", "snowy_beach", "forest", "wooded_hills", "flower_forest", "birch_forest", "birch_forest_hills", "tall_birch_forest", "tall_birch_hills", "dark_forest", "dark_forest_hills", "jungle", "jungle_hills", "modified_jungle", "jungle_edge", "modified_jungle_edge", "bamboo_jungle", "bamboo_jungle_hills", "taiga", "taiga_hills", "taiga_mountains", "snowy_taiga", "snowy_taiga_hills", "snowy_taiga_mountains", "giant_tree_taiga", "giant_tree_taiga_hills", "giant_spruce_taiga", "giant_spruce_taiga_hills", "mushroom_fields", "mushroom_field_shore", "swamp", "swamp_hills", "savanna", "savanna_plateau", "shattered_savanna", "shattered_savanna_plateau", "plains", "sunflower_plains", "desert", "desert_hills", "desert_lakes", "snowy_tundra", "snowy_mountains", "ice_spikes", "mountains", "wooded_mountains", "gravelly_mountains", "modified_gravelly_mountains", "mountain_edge", "badlands", "badlands_plateau", "modified_badlands_plateau", "wooded_badlands_plateau", "modified_wooded_badlands_plateau", "eroded_badlands", "dripstone_caves", "lush_caves", "nether_wastes", "crimson_forest", "warped_forest", "soul_sand_valley", "basalt_deltas", "the_end", "small_end_islands", "end_midlands", "end_highlands", "end_barrens", "the_void" };
		folderpath = png.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(png.path().filename().u8string().size()), folderpath.end());
		name = png.path().filename().u8string();
		name.erase(name.end() - 11, name.end());
		std::string temp, option, value, time1, last, height1, tempnum;
		nlohmann::json j, tempj;
		std::vector<nlohmann::json> v, tempv;
		std::vector<std::vector<int>> weights;
		std::vector<std::vector<std::pair<std::string, std::string>>> times, heights;
		std::vector<std::vector<std::string>> biomes, textures;
		std::vector<std::array<bool, 4>> weather;
		std::vector<std::pair<std::string, signed char>> names; // -1 = normal string, 0 = regex, 1 = iregex
		std::vector<int> baby, minheight, maxheight;
		std::stringstream ss;
		std::ifstream fin(png.path());
		while (fin)
		{
			std::getline(fin, temp);
			option.clear();
			value.clear();
			bool isvalue = false;
			if (temp == "" || temp.at(0) == '#')
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
				else if (isvalue)
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
			if (tempnum == "")
			{
				continue;
			}
			curnum = stoi(tempnum);
			if (static_cast<size_t>(curnum) > textures.size())
			{
				textures.resize(curnum);
				weights.resize(curnum);
				biomes.resize(curnum);
				times.resize(curnum);
				baby.resize(curnum, -1);
				heights.resize(curnum);
				names.resize(curnum, std::make_pair("", -1));
				weather.resize(curnum, { false, false, false, false });
				minheight.resize(curnum, INT_MIN);
				maxheight.resize(curnum, INT_MIN);
			}
			if (option.starts_with("textures.") || option.starts_with("skins."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (temp != "")
					{
						if (temp == "1")
						{
							textures.at(curnum - 1).push_back("minecraft:textures/entity/" + c8tomb(folderpath + name) + ".png");
						}
						else
						{
							textures.at(curnum - 1).push_back("minecraft:varied/textures/entity/" + c8tomb(folderpath + name) + temp + ".png");
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
					if (temp != "")
					{
						weights.at(curnum - 1).push_back(stoi(temp));
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
					if (temp != "")
					{
						if (temp.find(":") == std::string::npos) // does not contain a namespace
						{
							for (std::string& s : biomelist)
							{
								if (ununderscore(s) == lowercase(temp))
								{
									biomes.at(curnum - 1).push_back("minecraft:" + s);
									break;
								}
							}
						}
						else // contains a namespace
						{
							biomes.at(curnum - 1).push_back(temp);
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
					if (temp != "")
					{
						height1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + i);
								break;
							}
							height1 += temp.at(i);
						}
						heights.at(curnum - 1).push_back(std::make_pair(height1, temp));
					}
				}
			}
			else if (option.starts_with("minHeight"))
			{
				minheight.at(curnum - 1) = stoi(value);
			}
			else if (option.starts_with("maxHeight"))
			{
				maxheight.at(curnum - 1) = stoi(value);
			}
			else if (option.starts_with("name."))
			{
				// TODO: find out how non-regex works
				// pattern/ipattern to regex/iregex conversion
				temp = value;
				bool insensitive = false;
				if (temp.find("regex:") != std::string::npos)
				{
					if (temp.find("iregex:") != std::string::npos)
					{
						insensitive = true;
					}
					else
					{
						insensitive = false;
					}
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + i + 1);
							break;
						}
					}
				}
				else if (temp.find("pattern:") != std::string::npos)
				{
					if (temp.find("ipattern:") != std::string::npos)
					{
						insensitive = true;
					}
					else
					{
						insensitive = false;
					}
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + i + 1);
							break;
						}
					}
					temp = oftoregex(temp);
				}
				names.at(curnum - 1) = std::make_pair(temp, insensitive);
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
					baby.at(curnum - 1) = 1;
				}
				else if (value == "false")
				{
					baby.at(curnum - 1) = 0;
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
					if (temp != "")
					{
						time1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + i);
								break;
							}
							time1 += temp.at(i);
						}
						times.at(curnum - 1).push_back(std::make_pair(time1, temp));
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
						weather.at(curnum).at(1) = true;
						weather.at(curnum).at(0) = true;
					}
					if (temp == "rain")
					{
						weather.at(curnum).at(2) = true;
						weather.at(curnum).at(0) = true;
					}
					if (temp == "thunder")
					{
						weather.at(curnum).at(3) = true;
						weather.at(curnum).at(0) = true;
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
			if (weights.at(i).size())
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
			if (biomes.at(i).size())
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
			if (times.at(i).size())
			{
				// TODO: no daytime prop?
				/*for (int j = 0; j < times[i].size(); j++)
				{
					tempv.push_back({ {"type", "varied-mobs:time-prop"}, {"positions", nlohmann::json::array({times[i][j].first, times[i][j].second})}, {"choices", {tempj}} });
				}
				tempj = { {"type", "varied-mobs:seq"}, {"choices", tempv} };*/
			}
			if (heights.at(i).size())
			{
				tempv.clear();
				for (std::pair<std::string, std::string>& p : heights.at(i))
				{
					tempv.push_back({ {"above", p.first}, {"below", p.second}, {"then", tempj} });
				}
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
			if (names.at(i).first != "")
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
		std::ofstream fout(path + u8"/assets/minecraft/varied/textures/entity/" + folderpath + name + u8".json");
		fout << j.dump(1, '\t') << std::endl;
	}

public:
	bool success = false;

	// main vmt function
	inline vmt(const std::u8string& path, const std::u8string& filename)
	{
		// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
		// source: assets/minecraft/optifine/random/entity/
		// destination: assets/minecraft/varied/textures/entity/


		bool optifine, newlocation;
		std::u8string name;
		std::vector<int> numbers;
		if (std::filesystem::is_directory( + u8"/assets/minecraft/varied/textures/entity"))
		{
			if (autoreconvert)
			{
				out(3) << "VMT: Reconverting " << filename << std::endl;
				std::filesystem::remove_all(path + u8"/assets/minecraft/varied");
			}
			else
			{
				out(2) << "VMT: Varied Mob Textures folder found in " << filename << ", skipping" << std::endl;
				return;
			}
		}
		if (std::filesystem::is_directory(path + u8"/assets/minecraft/optifine/random/entity"))
		{
			optifine = true;
			newlocation = true;
		}
		else if (std::filesystem::is_directory(path + u8"/assets/minecraft/optifine/mob"))
		{
			optifine = true;
			newlocation = false;
		}
		else if (std::filesystem::is_directory(path + u8"/assets/minecraft/mcpatcher/mob"))
		{
			optifine = false;
		}
		else
		{
			out(2) << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		out(3) << "VMT: Converting Pack " << filename << std::endl;
		for (auto& png : std::filesystem::recursive_directory_iterator(path + u8"/assets/minecraft/" + (optifine ? u8"optifine" + std::u8string(newlocation ? u8"/random/entity/" : u8"/mob/") : u8"mcpatcher/mob/")))
		{
			if (png.path().extension() == ".png" || png.path().extension() == ".properties")
			{
				out(1) << u8"VMT: Converting " + png.path().filename().u8string() << std::endl;
			}
			if (png.path().filename().extension() == ".png")
			{
				vmtpng(name, path, newlocation, numbers, png);
			}
			else if (png.path().filename().extension() == ".properties")
			{
				vmtprop(path, newlocation, png);
			}
		}
		if (!numbers.empty())
		{
			std::vector<nlohmann::json> v;
			for (size_t i = 0; i < numbers.size(); i++)
			{
				v.push_back({ {"below", i + 1}, {"then", {{"type", std::string{VMT} + ":constant"}, {"identifier", "minecraft:varied/textures/entity/" + c8tomb(name) + std::to_string(numbers.at(i)) + ".png"}}} });
			}
			nlohmann::json j = { {"version", 1}, {"root", {{"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", { {"type", std::string{VMT} + ":range"}, {"when", {{"type", std::string{VMT} + ":random"}, {"min", 0}, {"max", numbers.size() + 1}}}, {"options", v} } }}} };

			if (!std::filesystem::exists(path + u8"/assets/minecraft/varied/textures/entity/" + name + u8".json"))
			{
				std::ofstream fout(path + u8"/assets/minecraft/varied/textures/entity/" + name + u8".json");
				fout << j.dump(1, '\t') << std::endl;
				fout.close();
			}
			numbers.clear();
		}
		success = true;
	}
};
