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
	std::vector<unsigned char> buffer, image, image1, image2, image3, top; // before h/2: bottom, top (rotate 90 clockwise), south; h/2 to h: west, north, east
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
			std::cout << "Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		if (zipa.HasEntry("assets/minecraft/optifine/sky/"))
		{
			optifine = true;
		}
		else if (zipa.HasEntry("assets/minecraft/mcpatcher/sky/"))
		{
			optifine = false;
		}
		else
		{
			std::cout << "Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		std::cout << "Extracting " << filename << std::endl;
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(path + "/assets/fabricskyboxes/sky", std::filesystem::path::format::auto_format)).exists())
		{
			std::cout << "Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
			return;
		}
		if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(path + "/assets/minecraft/optifine/sky", std::filesystem::path::format::auto_format)).exists())
		{
			optifine = true;
		}
		else if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(path + "/assets/minecraft/mcpatcher/sky", std::filesystem::path::format::auto_format)).exists())
		{
			optifine = false;
		}
		else
		{
			std::cout << "Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
	}
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
			error = lodepng::load_file(buffer, png.path().string());
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::decode(image, w, h, state, buffer);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			std::cout << "converting: " << png.path().filename() << std::endl;
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
			for (long long i = 0; i < (w * 4) / 3; i += 4)
			{
				for (long long j = 0; j < h / 2; j++)
				{
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 1]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 2]);
					top.push_back(image2[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 3]);
				}
			}
			buffer.clear();
			std::filesystem::create_directories(zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky" : path + "/assets/fabricskyboxes/sky");
			error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_bottom.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, top, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_top.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_south.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
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
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_west.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image2, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_north.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			buffer.clear();
			error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
			error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky/" : path + "/assets/fabricskyboxes/sky/") + filename + "_east.png");
			if (error)
			{
				std::cout << "png error: " << lodepng_error_text(error);
			}
		}
		else if (png.path().extension() == ".properties")
		{
			int startfadein, endfadein, startfadeout = -1, endfadeout;
			std::string name = png.path().filename().string(), source, option, value;
			name.erase(name.end() - 11, name.end());
			source = name;
			std::stringstream ss;
			nlohmann::json j = { {"schemaVersion", 2}, {"type", "square-textured"}, {"conditions", {{"worlds", {{"minecraft:overworld"}}}}} };
			std::ifstream fin(png.path().string());
			while (fin)
			{
				std::getline(fin, temp);
				for (int i = 0; i < temp.size(); i++) // quick and dirty way to seperate option from value: change '=' to ' ' then use stringstream
				{
					if (temp[i] == '=')
					{
						temp[i] = ' ';
					}
				}
				ss.str(temp);
				option.clear();
				value.clear();
				ss.clear();
				ss >> option;
				getline(ss, value);
				value.erase(value.begin());
				if (option == "source")
				{
					source = value;
					source.erase(source.end() - 4, source.end());
				}
				if (option == "startFadeIn")
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
					startfadein = (stoi(temp) + 18000) % 24000;
					j["properties"]["fade"]["startFadeIn"] = (stoi(temp) + 18000) % 24000;
				}
				if (option == "startFadeOut")
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
					startfadeout = (stoi(temp) + 18000) % 24000;
					j["properties"]["fade"]["startFadeOut"] = (stoi(temp) + 18000) % 24000;
				}
				if (option == "endFadeIn")
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
					endfadein = (stoi(temp) + 18000) % 24000;
					j["properties"]["fade"]["endFadeIn"] = (stoi(temp) + 18000) % 24000;
				}
				if (option == "endFadeOut")
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
					endfadeout = (stoi(temp) + 18000) % 24000;
					j["properties"]["fade"]["endFadeOut"] = (stoi(temp) + 18000) % 24000;
				}
				if (option == "blend")
				{
					j["properties"]["blend"]["type"] = value;
					j["blend"] = false;
					// ^ uncomment when new fsb releases and blend is fixed
				}
				if (option == "rotate")
				{
					j["properties"]["shouldrotate"] = (value == "true") ? true : false;
				}
				if (option == "speed")
				{
					// fsb doesn't have this yet (rotation speed)
				}
				if (option == "axis")
				{
					std::string x, y, z;
					std::stringstream axis;
					axis.str(value);
					axis >> x >> y >> z;
					j["properties"]["rotation"]["axis"] = { stod(x), stod(y), stod(z) };
				}
				if (option == "weather")
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
				if (option == "biomes")
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
					// dunno how this works either lol (will be changed when new ver of fsb is released)
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
			std::ifstream fin(png.path().string());
			zed.clear();
			while (fin)
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

int main()
{
	std::vector<std::string> paths;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::ifstream config("mcpppp.properties");
	if (config.fail())
	{
		std::ofstream createconfig("mcpppp.properties");
		createconfig << "# Insert path(s) of directories to search for resource packs" << std::endl;
		createconfig.close();
		std::cerr << "Config file not found" << std::endl;
#ifdef _WIN32
		system("pause"); // dont exit program otherwise ppl wont see the message
#else
		std::cout << "Press enter to continue . . .";
		getline(std::cin, str);
#endif
		return 0;
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
	std::error_code ec;
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
			}
			else if (entry.path().extension() == ".zip")
			{
				fsb(entry.path().string(), entry.path().filename().string(), true);
			}
		}
	}
#ifdef _WIN32
	system("pause");
#else
	std::cout << "Press enter to continue . . .";
	getline(std::cin, str);
#endif
}