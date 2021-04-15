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

bool pauseonexit = true, dolog = false, dotimestamp = false;
int outputlevel = 3, loglevel = 2;
std::ofstream logfile;

std::string lowercase(std::string str)
{
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] += 32;
		}
	}
	return str;
}

std::string timestamp()
{
	time_t timet = time(NULL);
	struct tm* timeinfo = localtime(&timet);
	std::string hour = std::to_string(timeinfo->tm_hour);
	if (hour.length() == 1)
	{
		hour.insert(hour.begin(), '0');
	}
	std::string min = std::to_string(timeinfo->tm_min);
	if (min.length() == 1)
	{
		min.insert(min.begin(), '0');
	}
	std::string sec = std::to_string(timeinfo->tm_sec);
	if (sec.length() == 1)
	{
		sec.insert(sec.begin(), '0');
	}
	return '[' + hour + ':' + min + ':' + sec + "] ";
}

std::string ununderscore(std::string str)
{
	std::string str2;
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] != '_')
		{
			str2 += str[i];
		}
	}
	return str2;
}

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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			}
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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "FSB: Extracting " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "FSB: Extracting " << filename << std::endl;
		}
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/fabricskyboxes/sky"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			}
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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "FSB: Converting Pack " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "FSB: Converting Pack " << filename << std::endl;
		}
	}
	for (auto& png : std::filesystem::directory_iterator(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0"))
	{
		if (png.path().extension() == ".png" || png.path().extension() == ".properties")
		{
			if (outputlevel <= 1)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "FSB: Converting " + png.path().filename().string() << std::endl;
			}
			if (logfile.good() && loglevel <= 1)
			{
				logfile << timestamp() << "FSB: Converting " + png.path().filename().string() << std::endl;
			}
		}
		if (png.path().extension() == ".png")
		{
			int error;
			std::string filename = png.path().filename().string();
			filename.erase(filename.end() - 4, filename.end());
			error = lodepng::load_file(buffer, png.path().string());
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::decode(image, w, h, state, buffer);
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
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
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_bottom.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			buffer.clear();
			error = lodepng::encode(buffer, top, h / 2, w / 3, state);
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_top.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_south.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
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
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_west.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			buffer.clear();
			error = lodepng::encode(buffer, image2, w / 3, h / 2, state);
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_north.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + filename + "_east.png");
			if (error)
			{
				std::cerr << (dotimestamp ? timestamp() : "") << "FSB: png error: " << lodepng_error_text(error);
				if (logfile.good())
				{
					logfile << timestamp() << "FSB: png error: " << lodepng_error_text(error);
				}
			}
			buffer.clear();
			buffer.shrink_to_fit();
			image.clear();
			image.shrink_to_fit();
			image1.clear();
			image1.shrink_to_fit();
			image2.clear();
			image2.shrink_to_fit();
			image3.clear();
			image3.shrink_to_fit();
			top.clear();
			top.shrink_to_fit();
			bottom.clear();
			bottom.shrink_to_fit();
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
			temp = source; // check if png file exists
			for (int i = 0; i < temp.size(); i++)
			{
				if (temp[i] == ':')
				{
					temp[i] = '/';
				}
			}
			if (!std::filesystem::exists((zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_top.png"))
			{
				image = { 0, 0, 0, 1 };
				lodepng::encode(buffer, image, 1, 1, state);
				lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_bottom.png");
				lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_north.png");
				lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_south.png");
				lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_west.png");
				lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_east.png");
				image.clear();
				image.shrink_to_fit();
				buffer.clear();
				buffer.shrink_to_fit();
			}
			std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + name + ".json");
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
	}
	if (zip)
	{
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "FSB: Compressing " + filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "FSB: Compressing " + filename << std::endl;
		}
		Zippy::ZipEntryData zed;
		long long filesize;
		for (auto& png : std::filesystem::directory_iterator("mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky"))
		{
			std::ifstream fin(png.path().string(), std::ios::binary | std::ios::ate);
			zed.clear();
			filesize = png.file_size();
			zed.resize(filesize);
			fin.seekg(0, std::ios::beg);
			fin.read((char*)(zed.data()), filesize);
			fin.close();
			zipa.AddEntry("assets/fabricskyboxes/sky/" + png.path().filename().string(), zed);
		}
		zed.clear();
		zed.shrink_to_fit();
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

	std::vector<std::string> biomelist = { "ocean", "deep_ocean", "frozen_ocean", "deep_frozen_ocean", "cold_ocean", "deep_cold_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean", "deep_warm_ocean", "river", "frozen_river", "beach", "stone_shore", "snowy_beach", "forest", "wooded_hills", "flower_forest", "birch_forest", "birch_forest_hills", "tall_birch_forest", "tall_birch_hills", "dark_forest", "dark_forest_hills", "jungle", "jungle_hills", "modified_jungle", "jungle_edge", "modified_jungle_edge", "bamboo_jungle", "bamboo_jungle_hills", "taiga", "taiga_hills", "taiga_mountains", "snowy_taiga", "snowy_taiga_hills", "snowy_taiga_mountains", "giant_tree_taiga", "giant_tree_taiga_hills", "giant_spruce_taiga", "giant_spruce_taiga_hills", "mushroom_fields", "mushroom_field_shore", "swamp", "swamp_hills", "savanna", "savanna_plateau", "shattered_savanna", "shattered_savanna_plateau", "plains", "sunflower_plains", "desert", "desert_hills", "desert_lakes", "snowy_tundra", "snowy_mountains", "ice_spikes", "mountains", "wooded_mountains", "gravelly_mountains", "modified_gravelly_mountains", "mountain_edge", "badlands", "badlands_plateau", "modified_badlands_plateau", "wooded_badlands_plateau", "modified_wooded_badlands_plateau", "eroded_badlands", "dripstone_caves", "lush_caves", "nether_wastes", "crimson_forest", "warped_forest", "soul_sand_valley", "basalt_deltas", "the_end", "small_end_islands", "end_midlands", "end_highlands", "end_barrens", "the_void" };
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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			}
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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "VMT: Extracting " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "VMT: Extracting " << filename << std::endl;
		}
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/"), "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/minecraft/varied/textures/entity"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && outputlevel <= 2)
			{
				logfile << timestamp() << "VMT: Variated Mob Textures folder found in " << filename << ", skipping" << std::endl;
			}
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
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && outputlevel <= 2)
			{
				logfile << timestamp() << "VMT: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "VMT: Converting Pack " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "VMT: Converting Pack " << filename << std::endl;
		}
	}
	for (auto& png : std::filesystem::recursive_directory_iterator((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/" + (optifine ? "optifine" + std::string(newlocation ? "/random/entity/" : "/mob/") : "mcpatcher/mob/")))
	{
		if (png.path().extension() == ".png" || png.path().extension() == ".properties")
		{
			if (outputlevel <= 1)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "VMT: Converting " + png.path().filename().string() << std::endl;
			}
			if (logfile.good() && loglevel <= 1)
			{
				logfile << timestamp() << "VMT: Converting " + png.path().filename().string() << std::endl;
			}
		}
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
					v.push_back("minecraft:varied/textures/entity/" + folderpath + name + std::to_string(i) + ".png");
				}
				v.push_back("minecraft:textures/entity/" + folderpath + name + ".png");
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
			std::string temp, option, value, time1, last, height1;
			nlohmann::json j, tempj;
			std::vector<nlohmann::json> v, tempv;
			std::vector<std::vector<int>> weights;
			std::vector<std::vector<std::pair<std::string, std::string>>> times, heights;
			std::vector<std::vector<std::string>> biomes, weather, textures;
			std::vector<std::string> names;
			std::vector<int> baby;
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
					baby.resize(stoi(curnum), -1);
					heights.resize(stoi(curnum));
					names.resize(stoi(curnum), "");
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
								textures[stoi(curnum) - 1].push_back("minecraft:textures/entity/" + folderpath2 + name2 + ".png");
							}
							else
							{
								textures[stoi(curnum) - 1].push_back("minecraft:varied/textures/entity/" + folderpath2 + name2 + temp + ".png");
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
					// TODO: actually find out how this works
					temp = value;
					if (temp.find("regex") != std::string::npos || temp.find("pattern") != std::string::npos)
					{
						for (int i = 0; i < temp.size(); i++)
						{
							if (temp[i] == ':')
							{
								temp.erase(temp.begin(), temp.begin() + i);
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
					tempj = { {"type", "varied-mobs:seq"}, {"choices", {tempv}} };
				}
				if (heights[i].size())
				{
					for (int j = 0; j < heights[i].size(); j++)
					{
						tempv.push_back({ {"type", "varied-mobs:y-prop"}, {"positions", nlohmann::json::array({heights[i][j].first, heights[i][j].second})}, {"choices", {tempj}} });
					}
					tempj = { {"type", "varied-mobs:seq"}, {"choices", {tempv}} };
				}
				if (names[i] != "")
				{
					tempj = { {"type", "varied-mobs:name"}, {"regex", names[i]}, {"value", tempj} };
				}
				v.push_back(tempj);
			}
			j = { {"type", "varied-mobs:pick"}, {"choices", v} };
			std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath2 + name2 + ".json");
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
			temp.clear();
			option.clear();
			value.clear();
			time1.clear();
			j.clear();
			tempj.clear();
			height1.clear();
			v.clear();
			v.shrink_to_fit();
			tempv.clear();
			tempv.shrink_to_fit();
			weights.clear();
			weights.shrink_to_fit();
			times.clear();
			times.shrink_to_fit();
			heights.clear();
			heights.shrink_to_fit();
			textures.clear();
			textures.shrink_to_fit();
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
		if (!std::filesystem::exists((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json"))
		{
			std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/varied/textures/entity/" + folderpath + name + ".json");
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
		numbers.clear();
	}
	curnum.clear();
	name.clear();
	name2.clear();
	folderpath.clear();
	folderpath2.clear();
	curname.clear();
	numbers.clear();
	numbers.shrink_to_fit();
	biomelist.clear();
	biomelist.shrink_to_fit();
	if (zip)
	{
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "VMT: Compressing " + filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "VMT: Compressing " + filename << std::endl;
		}
		std::string temp;
		Zippy::ZipEntryData zed;
		long long filesize;
		for (auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder + "/assets/minecraft/varied/textures/entity/"))
		{
			temp = png.path().string();
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
			std::ifstream fin(png.path().string(), std::ios::binary | std::ios::ate);
			zed.clear();
			filesize = png.file_size();
			zed.resize(filesize);
			fin.seekg(0, std::ios::beg);
			fin.read((char*)(zed.data()), filesize);
			fin.close();
			zipa.AddEntry(temp + png.path().filename().string() + (png.is_directory() ? "/" : ""), zed);
		}
		temp.clear();
		zed.clear();
		zed.shrink_to_fit();
		zipa.Save();
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}

int main()
{
	bool autodeletetemp = false;
	std::vector<std::string> paths;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::error_code ec;
	std::ifstream config("mcpppp.properties");
	std::cout.sync_with_stdio(false);
	if (config.fail())
	{
		std::ofstream createconfig("mcpppp.properties");
		createconfig << "# MCPPPP will search folders for resource packs (such as your resourcepacks folder) and will edit the resource pack.\n# It won't touch anything but the necessary folders, and will skip the resourcepack if the folders already exist.\n# Enter a newline-seperated list of such folders" << std::endl;
		createconfig.close();
		std::cerr << (dotimestamp ? timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
		goto exit;
	}
	while (config.good())
	{
		getline(config, str);
		if (str[0] == '/' && str[1] == '/')
		{
			ss.clear();
			ss.str(str);
			ss >> temp;
			if (temp == "//set")
			{
				ss >> option;
				getline(ss, value);
				value.erase(value.begin());
				if (lowercase(option) == "pauseonexit")
				{
					if (lowercase(value) == "true")
					{
						pauseonexit = true;
					}
					else if (lowercase(value) == "false")
					{
						pauseonexit = false;
					}
					else
					{
						std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
					}
				}
				else if (lowercase(option) == "log")
				{
					dolog = true;
					logfile.open(value);
				}
				else if (lowercase(option) == "timestamp")
				{
					if (lowercase(value) == "true")
					{
						dotimestamp = true;
					}
					else if (lowercase(value) == "false")
					{
						dotimestamp = false;
					}
					else
					{
						std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
					}
				}
				else if (lowercase(option) == "autodeletetemp")
				{
					if (lowercase(value) == "true")
					{
						autodeletetemp = true;
					}
					else if (lowercase(value) == "false")
					{
						autodeletetemp = false;
					}
					else
					{
						std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
					}
				}
				else if (lowercase(option) == "outputlevel")
				{
					try
					{

					}
					catch (std::exception e)
					{

					}
				}
				else
				{
					std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid option: " << option << std::endl;
				}
			}
		}
		else if (str[0] != '#')
		{
			paths.push_back(str);
		}
	}
	config.close();
	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (autodeletetemp)
		{
			if (outputlevel <= 4)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "Folder named \"mcpppp-temp\" found. Removing..." << std::endl;
			}
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
			if (outputlevel <= 4)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
			}
			goto exit;
		}
	}
	for (std::string path : paths)
	{
		if (!std::filesystem::is_directory(path, ec))
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
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
	if (outputlevel <= 3)
	{
		std::cout << (dotimestamp ? timestamp() : "") << "All Done!" << std::endl;
	}
exit:
	if (pauseonexit)
	{
#ifdef _WIN32
		system("pause");
#else
		std::cout << (dotimestamp ? timestamp() : "") << "Press enter to continue . . .";
		getline(std::cin, str);
#endif
	}
}