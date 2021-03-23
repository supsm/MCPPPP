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

int main()
{
	std::string path;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::vector<unsigned char> buffer, image, image1, image2, image3, top; // before h/2: bottom, top (rotate 90 clockwise), south; h/2 to h: west, north, east
	// rotation: w*h - w + 1, w*h - 2*w + 1, ..., w*h - h*w + 1, w*h - w + 2, w*h - 2*w + 2, ..., w*h - w + w, w*h - 2*w + w, ...
	unsigned int w, h;
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	std::ifstream config("mcpppp.properties");
	if (config.fail())
	{
		std::ofstream createconfig{ "mcpppp.properties" };
		std::cerr << "Config file not found" << std::endl;
#ifdef _WIN32
		system("pause"); // dont exit program otherwise ppl wont see the message
#else
		system("read -p \"Press any key to continue . . .\"");
#endif
		return 0;
	}
	while (config.good())
	{
		getline(config, str);
		ss.clear();
		ss.str(str);
		option.clear();
		value.clear();
		ss >> option;
		if (option == "path")
		{
			getline(ss, value);
			path = value;
			path.erase(path.begin());
		}
	}
	config.close();
	std::error_code ec;
	if (!std::filesystem::is_directory(path, ec))
	{
		std::cerr << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
#ifdef _WIN32
		system("pause");
#else
		system("read -p \"Press any key to continue . . .\"");
#endif
		return -1;
	}
	for (auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			bool optifine;
			if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(entry.path().string() + "/assets/fabricskyboxes/sky", std::filesystem::path::format::auto_format)).exists())
			{
				std::cout << "Fabricskyboxes folder found in " << entry.path().filename() << ", skipping" << std::endl;
				continue;
			}
			if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(entry.path().string() + "/assets/minecraft/optifine/sky", std::filesystem::path::format::auto_format)).exists())
			{
				optifine = true;
			}
			else if (std::filesystem::directory_entry::directory_entry(std::filesystem::path::path(entry.path().string() + "/assets/minecraft/mcpatcher/sky", std::filesystem::path::format::auto_format)).exists())
			{
				optifine = false;
			}
			else
			{
				std::cout << "Nothing to convert in " << entry.path().filename() << ", skipping" << std::endl;
				continue;
			}
			for (auto& png : std::filesystem::directory_iterator(entry.path().string() + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0"))
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
					std::filesystem::create_directories(entry.path().string() + "/assets/fabricskyboxes/sky");
					error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_bottom.png");
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
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_top.png");
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
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_south.png");
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
					error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_west.png");
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
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_north.png");
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
					error = lodepng::save_file(buffer, entry.path().string() + "/assets/fabricskyboxes/sky/" + filename + "_east.png");
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
				}
				else if (png.path().extension() == ".properties")
				{
					int startfadein, endfadein, startfadeout = -1, endfadeout;
					std::string name = png.path().filename().string(), source;
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
							//j["properties"]["blend"]["type"] = value;
							// blend is broken in fsb release, I will uncomment when it is fixed
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
					if (startfadeout == -1)
					{
						j["properties"]["fade"]["startFadeOut"] = endfadeout - endfadein + startfadein;
					}
					j["textures"]["top"] = source + "_top.png";
					j["textures"]["bottom"] = source + "_bottom.png";
					j["textures"]["north"] = source + "_north.png";
					j["textures"]["south"] = source + "_south.png";
					j["textures"]["west"] = source + "_west.png";
					j["textures"]["east"] = source + "_east.png";
					j["conditions"]["worlds"] = { "minecraft:overworld" };
					std::ofstream fout(entry.path().string() + "/assets/fabricskyboxes/sky/" + name + ".json");
					fout << j.dump(4) << std::endl;
					fout.close();
				}
			}
		}
		else if (entry.path().extension() == ".zip")
		{
			std::string folder;
			Zippy::ZipArchive zip;
			zip.Open(entry.path().string());
			if (zip.HasEntry("/assets/fabricskyboxes/sky"))
			{
				std::cout << "Fabricskyboxes folder found in " << entry.path().filename() << ", skipping" << std::endl;
				zip.Close();
				continue;
			}
			if (!zip.HasEntry("/assets/minecraft/optifine/sky"))
			{
				std::cout << "Nothing to convert in " << entry.path().filename() << ", skipping" << std::endl;
				zip.Close();
				continue;
			}
			folder = entry.path().string();
			folder.erase(folder.end() - 4, folder.end());
			folder += '/';
			zip.ExtractEntry("pack.mcmeta", folder);
			zip.ExtractEntry("assets/", folder);
			zip.Close();
			// this part is just copy pasted with a few things changed (might not work, dunno)
			for (auto& png : std::filesystem::directory_iterator(folder + "/assets/minecraft/optifine/sky/world0"))
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
					std::filesystem::create_directories(folder + "/assets/fabricskyboxes/sky");
					error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_bottom.png");
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
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_top.png");
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
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_south.png");
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
					error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_west.png");
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
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_north.png");
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
					error = lodepng::save_file(buffer, folder + "/assets/fabricskyboxes/sky/" + filename + "_east.png");
					if (error)
					{
						std::cout << "png error: " << lodepng_error_text(error);
					}
				}
				else if (png.path().extension() == ".properties")
				{
					int startfadein, endfadein, startfadeout = -1, endfadeout;
					std::string name = png.path().filename().string(), source;
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
							//j["properties"]["blend"]["type"] = value;
							// blend is broken in fsb release, I will uncomment when it is fixed
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
					if (startfadeout == -1)
					{
						j["properties"]["fade"]["startFadeOut"] = endfadeout - endfadein + startfadein;
					}
					j["textures"]["top"] = source + "_top.png";
					j["textures"]["bottom"] = source + "_bottom.png";
					j["textures"]["north"] = source + "_north.png";
					j["textures"]["south"] = source + "_south.png";
					j["textures"]["west"] = source + "_west.png";
					j["textures"]["east"] = source + "_east.png";
					j["conditions"]["worlds"] = { "minecraft:overworld" };
					std::ofstream fout(folder + "/assets/fabricskyboxes/sky/" + name + ".json");
					fout << j.dump(4) << std::endl;
					fout.close();
				}
			}
			// TODO: figure out how zippy works and put folder contents in a zip
		}
	}
#ifdef _WIN32
	system("pause");
#else
	system("read -p \"Press any key to continue . . .\"");
#endif
}