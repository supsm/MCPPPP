/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <climits>
#include <filesystem>
#include <string>
#include <vector>

#define VMT std::string("reselect") // will happen soon

void vmt(std::string path, std::string filename, bool zip); // main vmt function that calls everything else
void vmtprop(std::string& folder, std::string& path, bool& newlocation, bool& zip, std::filesystem::directory_entry png); // converts optifine properties to vmt properties json
void vmtpng(std::string& name, std::string& folder, std::string& path, bool& newlocation, bool& zip, std::vector<int>& numbers, std::filesystem::directory_entry png); // moves vmt pngs to new location

void vmtpng(std::string& name, std::string& folder, std::string& path, bool& newlocation, bool& zip, std::vector<int>& numbers, std::filesystem::directory_entry png)
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
				for (char& c : folderpath)
				{
					if (c == '\\')
					{
						c = '/';
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
		folderpath = png.path().u8string();
		for (char& c : folderpath)
		{
			if (c == '\\')
			{
				c = '/';
			}
		}
		folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
		folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
		numbers.push_back(stoi(curnum));
		supsm::copy(png.path(), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + png.path().filename().u8string()));
	}
	else if (!numbers.empty()) // TODO: what am i even doing wtf
	{
		std::vector<nlohmann::json> v;
		for (size_t i = 0; i < numbers.size(); i++)
		{
			v.push_back({ {"below", i + 1}, {"then", {{"type", VMT + ":constant"}, {"identifier", "minecraft:varied/textures/entity/" + folderpath + name + std::to_string(numbers[i]) + ".png"}}} });
		}
		nlohmann::json j = { {"version", 1}, {"root", {{"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", numbers.size() + 1}}}, {"options", v} } }}} };
		if (!std::filesystem::exists(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json")))
		{
			std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath));
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
	int curnum;
	std::string name, folderpath;
	std::vector<std::string> biomelist = { "ocean", "deep_ocean", "frozen_ocean", "deep_frozen_ocean", "cold_ocean", "deep_cold_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean", "deep_warm_ocean", "river", "frozen_river", "beach", "stone_shore", "snowy_beach", "forest", "wooded_hills", "flower_forest", "birch_forest", "birch_forest_hills", "tall_birch_forest", "tall_birch_hills", "dark_forest", "dark_forest_hills", "jungle", "jungle_hills", "modified_jungle", "jungle_edge", "modified_jungle_edge", "bamboo_jungle", "bamboo_jungle_hills", "taiga", "taiga_hills", "taiga_mountains", "snowy_taiga", "snowy_taiga_hills", "snowy_taiga_mountains", "giant_tree_taiga", "giant_tree_taiga_hills", "giant_spruce_taiga", "giant_spruce_taiga_hills", "mushroom_fields", "mushroom_field_shore", "swamp", "swamp_hills", "savanna", "savanna_plateau", "shattered_savanna", "shattered_savanna_plateau", "plains", "sunflower_plains", "desert", "desert_hills", "desert_lakes", "snowy_tundra", "snowy_mountains", "ice_spikes", "mountains", "wooded_mountains", "gravelly_mountains", "modified_gravelly_mountains", "mountain_edge", "badlands", "badlands_plateau", "modified_badlands_plateau", "wooded_badlands_plateau", "modified_wooded_badlands_plateau", "eroded_badlands", "dripstone_caves", "lush_caves", "nether_wastes", "crimson_forest", "warped_forest", "soul_sand_valley", "basalt_deltas", "the_end", "small_end_islands", "end_midlands", "end_highlands", "end_barrens", "the_void" };
	folderpath = png.path().u8string();
	for (char& c : folderpath)
	{
		if (c == '\\')
		{
			c = '/';
		}
	}
	folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
	folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
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
		if (temp == "" || temp[0] == '#')
		{
			continue;
		}
		for (char& c : temp)
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
		for (int i = option.size() - 1; i >= 0; i--)
		{
			if (option[i] >= '0' && option[i] <= '9')
			{
				tempnum.insert(tempnum.begin(), option[i]);
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
		if (curnum > int(textures.size()))
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
						textures[curnum - 1].push_back("minecraft:textures/entity/" + folderpath + name + ".png");
					}
					else
					{
						textures[curnum - 1].push_back("minecraft:varied/textures/entity/" + folderpath + name + temp + ".png");
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
					weights[curnum - 1].push_back(stoi(temp));
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
					if (temp.find(":") == std::string::npos) // does not contain a namespace
					{
						for (std::string& s : biomelist)
						{
							if (ununderscore(s) == lowercase(temp))
							{
								biomes[curnum - 1].push_back("minecraft:" + s);
								break;
							}
						}
					}
					else // contains a namespace
					{
						biomes[curnum - 1].push_back(temp);
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
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
						{
							temp.erase(temp.begin(), temp.begin() + i);
							break;
						}
						height1 += temp[i];
					}
					heights[curnum - 1].push_back(std::make_pair(height1, temp));
				}
			}
		}
		else if (option.find("minHeight") == 0)
		{
			minheight[curnum - 1] = stoi(value);
		}
		else if (option.find("maxHeight") == 0)
		{
			maxheight[curnum - 1] = stoi(value);
		}
		else if (option.find("name.") == 0)
		{
			// TODO: find out how non-regex works
			// pattern/ipattern to regex/iregex conversion
			temp = value;
			bool insensitive;
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
					if (temp[i] == ':')
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
					if (temp[i] == ':')
					{
						temp.erase(temp.begin(), temp.begin() + i + 1);
						break;
					}
				}
				temp = oftoregex(temp);
			}
			names[curnum - 1] = std::make_pair(temp, insensitive);
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
				baby[curnum - 1] = 1;
			}
			else if (value == "false")
			{
				baby[curnum - 1] = 0;
			}
		}
		else if (option.find("health.") == 0)
		{
			// TODO
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
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
						{
							temp.erase(temp.begin(), temp.begin() + i);
							break;
						}
						time1 += temp[i];
					}
					times[curnum - 1].push_back(std::make_pair(time1, temp));
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
					weather[curnum][1] = true;
					weather[curnum][0] = true;
				}
				if (temp == "rain")
				{
					weather[curnum][2] = true;
					weather[curnum][0] = true;
				}
				if (temp == "thunder")
				{
					weather[curnum][3] = true;
					weather[curnum][0] = true;
				}
			}
		}
	}
	for (size_t i = 0; i < textures.size(); i++) // TODO: there's probably a better way to do this
	{
		if (textures[i].empty())
		{
			continue;
		}
		if (weights[i].size())
		{
			int weightsum = 0;
			tempv.clear();
			for (size_t j = 0; j < textures[i].size(); j++)
			{
				weightsum += weights[i][j];
				tempv.push_back({ {"below", weightsum}, {"then", {{"type", VMT + ":constant"}, {"identifier", textures[i][j]}}} });
			}
			tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", weightsum}}}, {"options", tempv} };
		}
		else
		{
			tempv.clear();
			for (size_t j = 0; j < textures[i].size(); j++)
			{
				tempv.push_back({ {"below", j + 1}, {"then", {{"type", VMT + ":constant"}, {"identifier", textures[i][j]}}} });
			}
			tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", textures[i].size()}}}, {"options", tempv} };
		}
		if (biomes[i].size())
		{
			tempj = { {"type", VMT + ":string"}, {"when", {{"type", VMT + ":entity_biome"}}}, {"options", {{{"match", biomes[i]}, {"then", tempj}}}} };
		}
		if (baby[i] != -1)
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
		if (times[i].size())
		{
			// TODO: no daytime prop?
			/*for (int j = 0; j < times[i].size(); j++)
			{
				tempv.push_back({ {"type", "varied-mobs:time-prop"}, {"positions", nlohmann::json::array({times[i][j].first, times[i][j].second})}, {"choices", {tempj}} });
			}
			tempj = { {"type", "varied-mobs:seq"}, {"choices", tempv} };*/
		}
		if (heights[i].size())
		{
			tempv.clear();
			for (std::pair<std::string, std::string>& p : heights[i])
			{
				tempv.push_back({ {"above", p.first}, {"below", p.second}, {"then", tempj} });
			}
			tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":entity_y"}}}, {"options", tempv} };
		}
		if (minheight[i] != INT_MIN)
		{
			if (maxheight[i] == INT_MIN)
			{
				tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":entity_y"}}}, {"options", { {"above", minheight[i]}, {"then", tempj} }} };
			}
			else
			{
				tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":entity_y"}}}, {"options", { {"above", minheight[i]}, {"below", maxheight[i]}, {"then", tempj} }} };
			}
		}
		else if (maxheight[i] != INT_MIN)
		{
			tempj = { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":entity_y"}}}, {"options", {{"below", maxheight[i]}, {"then", tempj} }} };
		}
		if (weather[i][0])
		{
			// TODO: no weather prop?
			/*tempj = {{"type", "varied-mobs:weather-prop"}, {"positions", {0, 1, 2, 2}}, {"choices", {weather[i][1] ? tempj : nlohmann::json(), weather[i][2] ? tempj : nlohmann::json(), weather[i][3] ? tempj : nlohmann::json()}}};*/
		}
		if (names[i].first != "")
		{
			std::string type;
			switch (names[i].second)
			{
			case -1:
				type = "match"; // normal string
			case 0:
				type = "matchPattern"; // regex
			case 1:
				type = "iMatchPattern"; // iregex
			}
			tempj = { {"type", VMT + ":string"}, {"use", {{"type", VMT + ":entity_name"}}}, {"options", {{{type, names[i].first}, {"then", tempj}}}} };
		}
		v.push_back(tempj);
	}
	tempv.clear();
	for (size_t i = 0; i < v.size(); i++)
	{
		tempv.push_back({ {"below", i + 1}, {"then", v[i]} });
	}
	j = { {"version", 1}, {"root", {{"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", tempv}}} };
	std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath);
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
			if (autoreconvert)
			{
				out(3) << "VMT: Reconverting " << filename << std::endl;
				zipa.DeleteEntry("assets/minecraft/varied/");
			}
			else
			{
				out(2) << "VMT: Varied Mob Textures folder found in " << filename << ", skipping" << std::endl;
				return;
			}
		}
		if (zipa.HasEntry("assets/minecraft/optifine/random/entity/"))
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
			if (autoreconvert)
			{
				out(3) << "VMT: Reconverting " << filename << std::endl;
				std::filesystem::remove_all(std::filesystem::u8path(path + "/assets/minecraft/varied"));
			}
			else
			{
				out(2) << "VMT: Varied Mob Textures folder found in " << filename << ", skipping" << std::endl;
				return;
			}
		}
		if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/optifine/random/entity")))
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
		std::vector<nlohmann::json> v;
		for (size_t i = 0; i < numbers.size(); i++)
		{
			v.push_back({ {"below", i + 1}, {"then", {{"type", VMT + ":constant"}, {"identifier", "minecraft:varied/textures/entity/" + folderpath + name + std::to_string(numbers[i]) + ".png"}}} });
		}
		nlohmann::json j = { {"version", 1}, {"root", {{"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", v.size()}}}, {"options", { {"type", VMT + ":range"}, {"when", {{"type", VMT + ":random"}, {"min", 0}, {"max", numbers.size() + 1}}}, {"options", v} } }}} };

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
			for (char& c : temp)
			{
				if (c == '\\')
				{
					c = '/';
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
		zipa.Save();
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}
