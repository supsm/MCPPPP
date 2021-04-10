#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "json.hpp"
#include "lodepng.cpp"

#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif

#include "Zippy.hpp"

void fsb(std::string path, std::string filename, bool zip)
{
	std::string temp, folder;
	std::vector<unsigned char> buffer, image, image1, image2, image3, top, bottom; // before h/2: bottom (rotate 90 counterclockwise), top (rotate 90 clockwise), south; h/2 to h: west, north, east
	// rotation: w*h - w + 1, w*h - 2*w + 1, ..., w*h - h*w + 1, w*h - w + 2, w*h - 2*w + 2, ..., w*h - w + w, w*h - 2*w + w, ...
	unsigned int w, h;
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	bool optifine;
	Zippy::ZipArchive zipa;
	if (zip)
	{
		zipa.Open(path);
		if (zipa.HasEntry("assets/fabricskyboxes/sky/"))
		{
			std::cout << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		else if (zipa.HasEntry("assets/minecraft/optifine/sky/"))
		{
			optifine = true;
		}
		else if (zipa.HasEntry("assets/minecraft/mcpatcher/sky/"))
		{
			optifine = false;
		}
		else
		{
			std::cout << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		std::cout << "FSB: Extracting " << filename << std::endl;
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/fabricskyboxes/sky"))
		{
			std::cout << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/optifine/sky"))
		{
			optifine = true;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/mcpatcher/sky"))
		{
			optifine = false;
		}
		else
		{
			std::cout << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
	}
	std::cout << "FSB: Converting " + filename << std::endl;
	for (auto& png : std::filesystem::directory_iterator(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0"))
	{
		if (png.path().extension() == ".png")
		{
			int error;
			std::string filename = png.path().filename().string();
			filename.erase(filename.end() - 4, filename.end());
			buffer.clear();
			image.clear();
			image1.clear();
			image2.clear();
			image3.clear();
			top.clear();
			bottom.clear();
			error = lodepng::load_file(buffer, png.path().string());
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::decode(image, w, h, state, buffer);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			std::cout << "FSB: converting: " << png.path().filename() << std::endl;
			image1.reserve(buffer.size() / 6);
			image2.reserve(buffer.size() / 6);
			image3.reserve(buffer.size() / 6);
			for (long long i = 0; i < (w * 4) * h / 2; i++)
			{
				if (i % (w * 4) < (w * 4) / 3)
				{
					image1.push_back(image[i]);
				}
				else if (i % (w * 4) < 2 * (w * 4) / 3)
				{
					image2.push_back(image[i]);
				}
				else
				{
					image3.push_back(image[i]);
				}
			}
			long long max = 0;
			top.reserve(buffer.size() / 6);
			bottom.reserve(buffer.size() / 6);
			for (long long i = 0; i < (w * 4) / 3; i += 4)
			{
				for (long long j = 0; j < h / 2; j++)
				{
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 1]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 2]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 3]);
					bottom.push_back(image1[(w * 4) / 3 * (j + 1) - i]);
					bottom.push_back(image1[(w * 4) / 3 * (j + 1) - i + 1]);
					bottom.push_back(image1[(w * 4) / 3 * (j + 1) - i + 2]);
					bottom.push_back(image1[(w * 4) / 3 * (j + 1) - i + 3]);
				}
			}
			buffer.clear();
			std::filesystem::create_directories(zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky" : path + "/assets/fabricskyboxes/sky");
			error = lodepng::encode(buffer, bottom, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_bottom.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, top, h / 2, w / 3, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_top.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_south.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			image1.clear();
			image2.clear();
			image3.clear();
			for (long long i = (w * 4) * h / 2; i < (w * 4) * h; i++)
			{
				if (i % (w * 4) < (w * 4) / 3)
				{
					image1.push_back(image[i]);
				}
				else if (i % (w * 4) < 2 * (w * 4) / 3)
				{
					image2.push_back(image[i]);
				}
				else
				{
					image3.push_back(image[i]);
				}
			}
			buffer.clear();
			error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_west.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image2, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_north.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_east.png");
			if (error)
			{
				std::cout << "FSB: png error: " << lodepng_error_text(error);
			}
		}
		else if (png.path().extension() == ".properties")
		{
			int startfadein = -1, endfadein = -1, startfadeout = -1, endfadeout = -1;
			std::string name = png.path().filename().string(), source, option, value;
			name.erase(name.end() - 11, name.end());
			source = name;
			std::stringstream ss;
			nlohmann::json j = { {"schemaVersion", 2}, {"type", "square-textured"}, {"conditions", {{"worlds", {{"minecraft:overworld"}}}}}, {"blend", false}, {"properties", {{"blend", {{"type", "add"}}}}} };
			// change blend to true for fsb 0.5.0
			std::ifstream fin(png.path().string());
			while (fin)
			{
				std::getline(fin, temp);
				option.clear();
				value.clear();
				bool isvalue = false;
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
				if (temp == "")
				{
					continue;
				}
				if (option == "source")
				{
					source = value;
					source.erase(source.end() - 4, source.end());
				}
				else if (option == "startFadeIn" || option == "startFadeOut" || option == "endFadeIn" || option == "endFadeOut")
				{
					temp = value;
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == ':')
						{
							temp.erase(temp.begin() + i);
							i--;
						}
					}
					temp += '0';
					(option == "startFadeIn" || option == "startFadeOut") ? (option == "startFadeIn" ? startfadein : endfadein) : (option == "endFadeIn" ? endfadein : endfadeout) = (stoi(temp) + 18000) % 24000;
					j["properties"]["fade"][option] = (stoi(temp) + 18000) % 24000;
				}
				else if (option == "blend")
				{
					j["properties"]["blend"]["type"] = value;
				}
				else if (option == "rotate")
				{
					j["properties"]["shouldrotate"] = (value == "true") ? true : false;
				}
				else if (option == "speed")
				{
					//j["properties"]["rotation"]["rotationSpeed"] = stod(value);
					// uncomment for fsb 0.5.0
				}
				else if (option == "axis")
				{
					std::string x, y, z;
					std::stringstream axis;
					axis.str(value);
					axis >> x >> y >> z;
					j["properties"]["rotation"]["axis"] = { stod(x), stod(y), stod(z) };
					// multiply everything by 360 for fsb 0.5.0
				}
				else if (option == "weather")
				{
					std::string weather;
					std::stringstream weathers;
					std::vector<std::string> weatherlist;
					weathers.str(value);
					while (weathers)
					{
						weathers >> weather;
						weatherlist.push_back(weather);
					}
					j["conditions"]["weather"] = weatherlist;
				}
				else if (option == "biomes")
				{
					std::string biome;
					std::stringstream biomes;
					std::vector<std::string> biomelist;
					biomes.str(value);
					while (biomes)
					{
						biomes >> biome;
						biomelist.push_back(biome);
					}
				}
				if (option == "heights")
				{
					// dunno how this works lol
				}
				if (option == "transition")
				{
					// dunno how this works either lol (will be changed when new ver of fsb is released maybe)
				}
			}
			fin.close();
			if (startfadeout == -1)
			{
				j["properties"]["fade"]["startFadeOut"] = endfadeout - endfadein + startfadein;
			}
			if (source[0] == '.' && source[1] == '/')
			{
				source.erase(source.begin());
				source = "fabricskyboxes:sky" + source;
			}
			j["textures"]["top"] = source + "_top.png";
			j["textures"]["bottom"] = source + "_bottom.png";
			j["textures"]["north"] = source + "_north.png";
			j["textures"]["south"] = source + "_south.png";
			j["textures"]["west"] = source + "_west.png";
			j["textures"]["east"] = source + "_east.png";
			j["conditions"]["worlds"] = { "minecraft:overworld" };
			if (!zip)
			{
				std::ofstream fout(path + "/assets/fabricskyboxes/sky/" + name + ".json");
				fout << j.dump(1, '\t') << std::endl;
				fout.close();
			}
			else
			{
				zipa.AddEntry("assets/fabricskyboxes/sky/" + name + ".json", j.dump(1, '\t') + '\n');
			}
		}
	}
	if (zip)
	{
		for (auto& png : std::filesystem::directory_iterator("mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky"))
		{
			unsigned char ch;
			Zippy::ZipEntryData zed;
			std::ifstream fin(png.path().string(), std::ios::binary);
			zed.clear();
			while (fin.good())
			{
				fin >> std::noskipws >> ch;
				zed.push_back(ch);
			}
			fin.close();
			zipa.AddEntry("assets/fabricskyboxes/sky/" + png.path().filename().string(), zed);
		}
	}
	if (zip)
	{
		zipa.Save();
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}

void vmt(std::string path, std::string filename, bool zip)
{
	// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
	// source: assets/minecraft/optifine/random/entity/
	// destination: assets/minecraft/varied/textures/entity/

	bool optifine, newlocation;
	std::string folder;
	Zippy::ZipArchive zipa;
	std::string curnum, name, name2, folderpath, folderpath2, curname;
	std::vector<int> numbers;
	if (zip)
	{
		zipa.Open(path);
		if (zipa.HasEntry("assets/minecraft/varied/textures/entity/"))
		{
			std::cout << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
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
			std::cout << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		std::cout << "VMT: Extracting " << filename << std::endl;
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/"), "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/minecraft/varied/textures/entity"))
		{
			std::cout << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/optifine/random/entity"))
		{
			optifine = true;
			newlocation = true;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/optifine/mob"))
		{
			optifine = true;
			newlocation = false;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/mcpatcher/mob"))
		{
			optifine = false;
		}
		else
		{
			std::cout << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
	}
	std::cout << "VMT: Converting " + filename << std::endl;
	for (auto& png : std::filesystem::recursive_directory_iterator((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/" + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/")))
	{

		if (png.path().filename().extension() == ".png")
		{
		vmtconvert:
			curnum.clear();
			for (int i = png.path().filename().string().size() - 5; i >= 0; i--)
			{
				if (png.path().filename().string()[i] >= '0' && png.path().filename().string()[i] <= '9')
				{
					curnum.insert(curnum.begin(), png.path().filename().string()[i]);
				}
				else
				{
					if (numbers.size() == 0)
					{
						name = png.path().filename().string();
						name.erase(name.begin() + i + 1, name.end());
						folderpath = png.path().string();
						folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
						folderpath.erase(folderpath.end() - png.path().filename().string().size(), folderpath.end());
						for (int i = 0; i < folderpath.size(); i++)
						{
							if (folderpath[i] == '\\')
							{
								folderpath[i] = '/';
							}
						}
					}
					curname = png.path().filename().string();
					curname.erase(curname.begin() + i + 1, curname.end());
					break;
				}
			}
			if (curname == name && curnum != "")
			{
				numbers.push_back(stoi(curnum));
				std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath);
				std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + png.path().filename().string());
			}
			else if (!numbers.empty())
			{
				std::vector<std::string> v;
				for (int i : numbers)
				{
					v.push_back("assets/minecraft/varied/textures/entity/" + folderpath + name + std::to_string(i) + ".png");
				}
				v.push_back("assets/minecraft/textures/entity/" + folderpath + name + ".png");
				nlohmann::json j = { {"type", "varied-mobs:pick"}, {"choices", v} };
				if (!std::filesystem::exists((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"))
				{
					std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json");
					fout << j.dump(1, '\t') << std::endl;
					fout.close();
				}
				numbers.clear();
				goto vmtconvert;
			}
		}
		else if (png.path().filename().extension() == ".properties")
		{
			folderpath2 = png.path().string();
			folderpath2.erase(folderpath2.begin(), folderpath2.begin() + folderpath2.rfind(newlocation ? "/random/entity/" : "/mob/") + (newlocation ? 15 : 5));
			folderpath2.erase(folderpath2.end() - png.path().filename().string().size(), folderpath2.end());
			for (int i = 0; i < folderpath2.size(); i++)
			{
				if (folderpath2[i] == '\\')
				{
					folderpath2[i] = '/';
				}
			}
			name2 = png.path().filename().string();
			name2.erase(name2.end() - 11, name.end());
			std::string temp, option, value, time1;
			nlohmann::json j, tempj;
			std::vector<nlohmann::json> v, tempv;
			std::vector<std::vector<int>> weights;
			std::vector<std::vector<std::pair<std::string, std::string>>> times;
			std::vector<std::vector<std::string>> biomes, weather, textures;
			std::stringstream ss;
			std::ifstream fin(png.path().string());
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
				}
				if (option.find("textures.") == 0 || option.find("skins.") == 0)
				{
					ss.clear();
					ss.str(value);
					while (ss)
					{
						ss >> temp;
						if (temp == "1")
						{
							textures[stoi(curnum) - 1].push_back("assets/minecraft/textures/entity/" + folderpath2 + name2);
						}
						else
						{
							textures[stoi(curnum) - 1].push_back("assets/minecraft/varied/textures/entity/" + folderpath2 + name2 + temp);
						}
					}
				}
				else if (option.find("weights.") == 0)
				{
					ss.clear();
					ss.str(value);
					while (ss)
					{
						ss >> temp;
						weights[stoi(curnum) - 1].push_back(stoi(temp));
					}
				}
				else if (option.find("biomes.") == 0)
				{
					ss.clear();
					ss.str(value);
					while (ss)
					{
						ss >> temp;
						biomes[stoi(curnum) - 1].push_back(temp);
					}
				}
				else if (option.find("heights.") == 0)
				{
					// don't really know how this works (of), will find out (TODO)
				}
				else if (option.find("name.") == 0)
				{
					// don't really understand how this works yet
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
					// TODO: find out how varied-mobs:not works
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
						ss >> temp;
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
				else if (option.find("weather.") == 0)
				{
					// not sure this is possible
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
					tempj = { {"type", "varied-mobs:biome"}, {"biomes", biomes[i]}, {"value", tempj} };
				}
				if (times[i].size())
				{
					for (int j = 0; j < times[i].size(); j++)
					{
						tempv.push_back({ {"type", "varied-mobs:time-prop"}, {"positions", nlohmann::json::object({times[i][j].first, times[i][j].second})}, {"choices", {tempj}} });
					}
					tempj = { {"type", "varied-mobs:seq"}, {"choices", {tempv}} };
				}
			}
			j = { {"type", "varied-mobs:pick"}, {"choices", {tempj}} };
			std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath2 + name2 + ".json");
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
	}
	if (!numbers.empty())
	{
		std::vector<std::string> v;
		for (int i : numbers)
		{
			v.push_back("assets/minecraft/varied/textures/entity/" + folderpath + name + std::to_string(i) + ".png");
		}
		v.push_back("assets/minecraft/textures/entity/" + folderpath + name + ".png");
		nlohmann::json j = { {"type", "varied-mobs:pick"}, {"choices", v} };
		if (!std::filesystem::exists((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"))
		{
			std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json");
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
		numbers.clear();
	}
	if (zip)
	{
		for (auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder + "/assets/minecraft/varied/textures/entity/"))
		{
			int tempint;
			unsigned char ch;
			std::string temp = png.path().string();
			temp.erase(temp.begin(), temp.begin() + folder.size() + 13);
			temp.erase(temp.end() - png.path().filename().string().size() - 1, temp.end()); // zippy doesnt like mixing \\ and /
			temp += '/';
			for (int i = 0; i < temp.size(); i++)
			{
				if (temp[i] == '\\')
				{
					temp[i] = '/';
				}
			}
			Zippy::ZipEntryData zed;
			std::ifstream fin(png.path().string(), std::ios::binary);
			fin.sync_with_stdio(false);
			zed.clear();
			while (fin.good())
			{
				fin >> std::noskipws >> ch;
				zed.push_back(ch);
			}
			fin.close();
			zipa.AddEntry(temp + png.path().filename().string() + (png.is_directory() ? "/" : ""), zed);
		}
	}
	if (zip)
	{
		zipa.Save();
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}

int main()
{
	std::vector<std::string> paths;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::error_code ec;
	std::ifstream config("mcpppp.properties");
	if (config.fail())
	{
		std::ofstream createconfig("mcpppp.properties");
		createconfig << "# MCPPPP will search folders for resource packs (such as your resourcepacks folder) and will edit the resource pack.\n# It won't touch anything but the necessary folders, and will skip the resourcepack if the folders already exist.\n# Enter a newline-seperated list of such folders" << std::endl;
		createconfig.close();
		std::cerr << "Config file not found, look for mcpppp.properties" << std::endl;
		goto exit;
	}
	while (config.good())
	{
		getline(config, str);
		if (str[0] != '#')
		{
			paths.push_back(str);
		}
	}
	config.close();
	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		std::cout << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
		goto exit;
	}
	for (std::string path : paths)
	{
		if (!std::filesystem::is_directory(path, ec))
		{
			std::cerr << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
			continue;
		}
		for (auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				fsb(entry.path().string(), entry.path().filename().string(), false);
				vmt(entry.path().string(), entry.path().filename().string(), false);
			}
			else if (entry.path().extension() == ".zip")
			{
				fsb(entry.path().string(), entry.path().filename().string(), true);
				vmt(entry.path().string(), entry.path().filename().string(), true);
			}
		}
	}
	std::cout << "All Done!" << std::endl;
exit:
#ifdef _WIN32
	system("pause");
#else
	std::cout << "Press enter to continue . . .";
	getline(std::cin, str);
#endif
}