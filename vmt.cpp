/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <filesystem>
#include <string>
#include <vector>

void vmtpng(std::string& name, std::string& folder, std::string& path, bool& newlocation, bool& zip, std::vector<int> numbers, std::filesystem::directory_entry png)
{
	std::string folderpath, curname, curnum;
vmtconvert:
	curnum.clear();
	for (int i = png.path().filename().u8string().size() - 5; i >= 0; i--)
	{
		if (png.path().filename().u8string()[i] >= '0' && png.path().filename().u8string()[i] <= '9')
		{
			curnum.insert(curnum.begin(), png.path().filename().u8string()[i]);
		}
		else
		{
			if (numbers.size() == 0)
			{
				name = png.path().filename().u8string();
				name.erase(name.begin() + i + 1, name.end());
				folderpath = png.path().u8string();
				for (int i = 0; i < folderpath.size(); i++)
				{
					if (folderpath[i] == '\\')
					{
						folderpath[i] = '/';
					}
				}
				folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
				folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
			}
			curname = png.path().filename().u8string();
			curname.erase(curname.begin() + i + 1, curname.end());
			break;
		}
	}
	if (curname == name && curnum != "")
	{
		numbers.push_back(stoi(curnum));
		std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath));
		supsm::copy(png.path(), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + png.path().filename().u8string()));
	}
	else if (!numbers.empty())
	{
		std::vector<std::string> v;
		for (int i : numbers)
		{
			v.push_back("minecraft:varied/textures/entity/" + folderpath + name + std::to_string(i) + ".png");
		}
		v.push_back("minecraft:textures/entity/" + folderpath + name + ".png");
		nlohmann::json j = { {"type", "varied-mobs:pick"}, {"choices", v} };
		if (!std::filesystem::exists(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json")))
		{
			std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"));
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
		numbers.clear();
		goto vmtconvert;
	}
}

void vmtprop(std::string& folder, std::string& path, bool& newlocation, bool& zip, std::filesystem::directory_entry png)
{
	std::string name, folderpath, curnum;
	std::vector<std::string> biomelist = { "ocean", "deep_ocean", "frozen_ocean", "deep_frozen_ocean", "cold_ocean", "deep_cold_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean", "deep_warm_ocean", "river", "frozen_river", "beach", "stone_shore", "snowy_beach", "forest", "wooded_hills", "flower_forest", "birch_forest", "birch_forest_hills", "tall_birch_forest", "tall_birch_hills", "dark_forest", "dark_forest_hills", "jungle", "jungle_hills", "modified_jungle", "jungle_edge", "modified_jungle_edge", "bamboo_jungle", "bamboo_jungle_hills", "taiga", "taiga_hills", "taiga_mountains", "snowy_taiga", "snowy_taiga_hills", "snowy_taiga_mountains", "giant_tree_taiga", "giant_tree_taiga_hills", "giant_spruce_taiga", "giant_spruce_taiga_hills", "mushroom_fields", "mushroom_field_shore", "swamp", "swamp_hills", "savanna", "savanna_plateau", "shattered_savanna", "shattered_savanna_plateau", "plains", "sunflower_plains", "desert", "desert_hills", "desert_lakes", "snowy_tundra", "snowy_mountains", "ice_spikes", "mountains", "wooded_mountains", "gravelly_mountains", "modified_gravelly_mountains", "mountain_edge", "badlands", "badlands_plateau", "modified_badlands_plateau", "wooded_badlands_plateau", "modified_wooded_badlands_plateau", "eroded_badlands", "dripstone_caves", "lush_caves", "nether_wastes", "crimson_forest", "warped_forest", "soul_sand_valley", "basalt_deltas", "the_end", "small_end_islands", "end_midlands", "end_highlands", "end_barrens", "the_void" };
	folderpath = png.path().u8string();
	for (int i = 0; i < folderpath.size(); i++)
	{
		if (folderpath[i] == '\\')
		{
			folderpath[i] = '/';
		}
	}
	folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
	folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
	name = png.path().filename().u8string();
	name.erase(name.end() - 11, name.end());
	std::string temp, option, value, time1, last, height1;
	nlohmann::json j, tempj;
	std::vector<nlohmann::json> v, tempv;
	std::vector<std::vector<int>> weights;
	std::vector<std::vector<std::pair<std::string, std::string>>> times, heights;
	std::vector<std::vector<std::string>> biomes, textures;
	std::vector<std::array<bool, 4>> weather;
	std::vector<std::string> names;
	std::vector<int> baby;
	std::stringstream ss;
	std::ifstream fin(png.path());
	while (fin)
	{
		std::getline(fin, temp);
		option.clear();
		value.clear();
		bool isvalue = false;
		if (temp == "" || temp[0] == '#')
		{
			continue;
		}
		for (int i = 0; i < temp.size(); i++)
		{
			if (temp[i] == '=')
			{
				isvalue = true;
			}
			else if (!isvalue)
			{
				option += temp[i];
			}
			else if (isvalue)
			{
				value += temp[i];
			}
		}
		curnum.clear();
		for (int i = option.size() - 1; i >= 0; i--)
		{
			if (option[i] >= '0' && option[i] <= '9')
			{
				curnum.insert(curnum.begin(), option[i]);
			}
			else
			{
				break;
			}
		}
		if (curnum.empty())
		{
			continue;
		}
		if (stoi(curnum) > textures.size())
		{
			textures.resize(stoi(curnum));
			weights.resize(stoi(curnum));
			biomes.resize(stoi(curnum));
			times.resize(stoi(curnum));
			baby.resize(stoi(curnum), -1);
			heights.resize(stoi(curnum));
			names.resize(stoi(curnum), "");
			weather.resize(stoi(curnum), { false, false, false, false });
		}
		if (option.find("textures.") == 0 || option.find("skins.") == 0)
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
						textures[stoi(curnum) - 1].push_back("minecraft:textures/entity/" + folderpath + name + ".png");
					}
					else
					{
						textures[stoi(curnum) - 1].push_back("minecraft:varied/textures/entity/" + folderpath + name + temp + ".png");
					}
				}
			}
		}
		else if (option.find("weights.") == 0)
		{
			ss.clear();
			ss.str(value);
			while (ss)
			{
				temp = "";
				ss >> temp;
				if (temp != "")
				{
					weights[stoi(curnum) - 1].push_back(stoi(temp));
				}
			}
		}
		else if (option.find("biomes.") == 0)
		{
			ss.clear();
			ss.str(value);
			while (ss)
			{
				temp = "";
				ss >> temp;
				if (temp != "")
				{
					for (int i = 0; i < biomelist.size(); i++)
					{
						if (ununderscore(biomelist[i]) == lowercase(temp))
						{
							biomes[stoi(curnum) - 1].push_back(biomelist[i]);
							break;
						}
					}
				}
			}
		}
		else if (option.find("heights.") == 0)
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
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
						{
							temp.erase(temp.begin(), temp.begin() + i);
							break;
						}
						height1 += temp[i];
					}
					heights[stoi(curnum) - 1].push_back(std::make_pair(height1, temp));
				}
			}
		}
		else if (option.find("name.") == 0)
		{
			// TODO: find out how non-regex works
			temp = value;
			if (temp.find("regex:") != std::string::npos || temp.find("pattern:") != std::string::npos)
			{
				for (int i = 0; i < temp.size(); i++)
				{
					if (temp[i] == ':')
					{
						temp.erase(temp.begin(), temp.begin() + i + 1);
						break;
					}
				}
			}
			names[stoi(curnum) - 1] = temp;
		}
		else if (option.find("professions.") == 0)
		{
			// not sure this is possible
		}
		else if (option.find("collarColors.") == 0)
		{
			// not sure this is possible
		}
		else if (option.find("baby.") == 0)
		{
			if (value == "true")
			{
				baby[stoi(curnum) - 1] = 1;
			}
			else if (value == "false")
			{
				baby[stoi(curnum) - 1] = 0;
			}
		}
		else if (option.find("health.") == 0)
		{
			// vmt doesn't accept health values, only %
		}
		else if (option.find("moonPhase.") == 0)
		{
			// not sure this is possible
		}
		else if (option.find("dayTime.") == 0)
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
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
						{
							temp.erase(temp.begin(), temp.begin() + i);
							break;
						}
						time1 += temp[i];
					}
					times[stoi(curnum) - 1].push_back(std::make_pair(time1, temp));
				}
			}
		}
		else if (option.find("weather.") == 0)
		{
			ss.clear();
			ss.str(value);
			while (ss)
			{
				temp = "";
				ss >> temp;
				if (temp == "clear")
				{
					weather[stoi(curnum)][1] = true;
					weather[stoi(curnum)][0] = true;
				}
				if (temp == "rain")
				{
					weather[stoi(curnum)][2] = true;
					weather[stoi(curnum)][0] = true;
				}
				if (temp == "thunder")
				{
					weather[stoi(curnum)][3] = true;
					weather[stoi(curnum)][0] = true;
				}
			}
		}
	}
	for (int i = 0; i < textures.size(); i++) // TODO: there's probably a better way to do this
	{
		if (textures[i].empty())
		{
			continue;
		}
		if (weights[i].size())
		{
			tempj = { {"type", "varied-mobs:pick"}, {"weights", weights[i]}, {"choices", textures[i]} };
		}
		else
		{
			tempj = { {"type", "varied-mobs:pick"}, {"choices", textures[i]} };
		}
		if (biomes[i].size())
		{
			tempj = { {"type", "varied-mobs:biome"}, {"biome", biomes[i]}, {"value", tempj} };
		}
		if (baby[i] != -1)
		{
			if (baby[i])
			{
				tempj = { {"type", "varied-mobs:baby"}, {"value", tempj} };
			}
			else
			{
				tempj = { {"type", "varied-mobs:not"}, {"value", {{"type", "varied-mobs:seq"}, {"choices", {nlohmann::json({{"type", "varied-mobs:baby"}, {"value", ""}}), tempj}}}} };
			}
		}
		if (times[i].size())
		{
			for (int j = 0; j < times[i].size(); j++)
			{
				tempv.push_back({ {"type", "varied-mobs:time-prop"}, {"positions", nlohmann::json::array({times[i][j].first, times[i][j].second})}, {"choices", {tempj}} });
			}
			tempj = { {"type", "varied-mobs:seq"}, {"choices", tempv} };
		}
		if (heights[i].size())
		{
			for (int j = 0; j < heights[i].size(); j++)
			{
				tempv.push_back({ {"type", "varied-mobs:y-prop"}, {"positions", nlohmann::json::array({heights[i][j].first, heights[i][j].second})}, {"choices", {tempj}} });
			}
			tempj = { {"type", "varied-mobs:seq"}, {"choices", tempv} };
		}
		if (weather[i][0])
		{
			tempj = { {"type", "varied-mobs:weather-prop"}, {"positions", {0, 1, 2, 2}}, {"choices", {weather[i][1] ? tempj : nlohmann::json(), weather[i][2] ? tempj : nlohmann::json(), weather[i][3] ? tempj : nlohmann::json()}} };
		}
		if (names[i] != "")
		{
			//tempj = { {"type", "varied-mobs:name"}, {"regex", names[i]}, {"value", tempj} };
		}
		else // names do not work in vmt until my pr gets accepted :)
		{
			v.push_back(tempj);
		}
	}
	j = { {"type", "varied-mobs:pick"}, {"choices", v} };
	std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"));
	fout << j.dump(1, '\t') << std::endl;
}

void vmt(std::string path, std::string filename, bool zip)
{
	// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
	// source: assets/minecraft/optifine/random/entity/
	// destination: assets/minecraft/varied/textures/entity/


	bool optifine, newlocation;
	std::string name, folder, folderpath;
	Zippy::ZipArchive zipa;
	std::vector<int> numbers;
	if (zip)
	{
		zipa.Open(path);
		if (zipa.HasEntry("assets/minecraft/varied/textures/entity/"))
		{
			out(2) << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		else if (zipa.HasEntry("assets/minecraft/optifine/random/entity/"))
		{
			optifine = true;
			newlocation = true;
		}
		else if (zipa.HasEntry("assets/minecraft/optifine/mob/"))
		{
			optifine = true;
			newlocation = false;
		}
		else if (zipa.HasEntry("assets/minecraft/mcpatcher/mob/"))
		{
			optifine = false;
		}
		else
		{
			out(2) << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		out(3) << "VMT: Extracting " << filename << std::endl;
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/"), "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/varied/textures/entity")))
		{
			out(2) << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		else if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/optifine/random/entity")))
		{
			optifine = true;
			newlocation = true;
		}
		else if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/optifine/mob")))
		{
			optifine = true;
			newlocation = false;
		}
		else if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/mcpatcher/mob")))
		{
			optifine = false;
		}
		else
		{
			out(2) << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		out(3) << "VMT: Converting Pack " << filename << std::endl;
	}
	for (auto& png : std::filesystem::recursive_directory_iterator(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/" + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/"))))
	{
		if (png.path().extension() == ".png" || png.path().extension() == ".properties")
		{
			out(1) << "VMT: Converting " + png.path().filename().u8string() << std::endl;
		}
		if (png.path().filename().extension() == ".png")
		{
			vmtpng(name, folder, path, newlocation, zip, numbers, png);
		}
		else if (png.path().filename().extension() == ".properties")
		{
			vmtprop(folder, path, newlocation, zip, png);
		}
	}
	if (!numbers.empty())
	{
		std::vector<std::string> v;
		for (int i : numbers)
		{
			v.push_back("minecraft:varied/textures/entity/" + folderpath + name + std::to_string(i) + ".png");
		}
		v.push_back("minecraft:textures/entity/" + folderpath + name + ".png");
		nlohmann::json j = { {"type", "varied-mobs:pick"}, {"choices", v} };
		if (!std::filesystem::exists(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json")))
		{
			std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"));
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
		numbers.clear();
	}
	folderpath.clear();
	numbers.clear();
	numbers.shrink_to_fit();
	if (zip)
	{
		out(3) << "VMT: Compressing " + filename << std::endl;
		std::string temp;
		Zippy::ZipEntryData zed;
		long long filesize;
		for (auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder + "/assets/minecraft/varied/textures/entity/"))
		{
			if (png.is_directory())
			{
				continue;
			}
			temp = png.path().u8string();
			temp.erase(temp.begin(), temp.begin() + folder.size() + 13);
			temp.erase(temp.end() - png.path().filename().u8string().size() - 1, temp.end()); // zippy doesnt like mixing \\ and /
			temp += '/';
			for (int i = 0; i < temp.size(); i++)
			{
				if (temp[i] == '\\')
				{
					temp[i] = '/';
				}
			}
			std::ifstream fin(png.path(), std::ios::binary | std::ios::ate);
			zed.clear();
			filesize = png.file_size();
			zed.resize(filesize);
			fin.seekg(0, std::ios::beg);
			fin.read((char*)(zed.data()), filesize);
			fin.close();
			zipa.AddEntry(temp + png.path().filename().u8string() + (png.is_directory() ? "/" : ""), zed);
		}
		temp.clear();
		zed.clear();
		zed.shrink_to_fit();
		if (deletesource)
		{
			zipa.DeleteEntry("assets/minecraft/" + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/"));
		}
		zipa.Save();
	}
	else if (deletesource)
	{
		std::filesystem::remove_all(std::filesystem::u8path(path + "/assets/minecraft/" + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/")));
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}
