/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <filesystem>
#include <string>
#include <vector>

void fsbpng(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	int error;
	unsigned int w, h;
	std::vector<unsigned char> buffer, image, image1, image2, image3, top, bottom; // before h/2: bottom (rotate 90 counterclockwise), top (rotate 90 clockwise), south; h/2 to h: west, north, east
	// rotation: w*h - w + 1, w*h - 2*w + 1, ..., w*h - h*w + 1, w*h - w + 2, w*h - 2*w + 2, ..., w*h - w + w, w*h - 2*w + w, ...
	std::string filename = png.path().filename().string();
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
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
}

void fsbprop(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	int startfadein = -1, endfadein = -1, startfadeout = -1, endfadeout = -1;
	std::string name = png.path().filename().string(), source, option, value, temp;
	std::vector<unsigned char> buffer;
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
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
			(option == "startFadeIn" || option == "startFadeOut") ? (option == "startFadeIn" ? startfadein : endfadein) : (option == "endFadeIn" ? endfadein : endfadeout) = (stoi(temp) + 18000 + 24000) % 24000;
			j["properties"]["fade"][option] = (stoi(temp) + 18000 + 24000) % 24000;
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
		j["properties"]["fade"]["startFadeOut"] = (endfadeout - endfadein + startfadein + 24000) % 24000;
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
		lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
		lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_bottom.png");
		lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_north.png");
		lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_south.png");
		lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_west.png");
		lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/" + temp + "_east.png");
		buffer.clear();
		buffer.shrink_to_fit();
	}
	std::ofstream fout((zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + name + ".json");
	fout << j.dump(1, '\t') << std::endl;
	fout.close();
}

void fsb(std::string path, std::string filename, bool zip)
{
	std::string folder;
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
			fsbpng(folder, path, zip, png);
		}
		else if (png.path().extension() == ".properties")
		{
			fsbprop(folder, path, zip, png);
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