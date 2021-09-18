/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

void rgb2hsv(double& first, double& second, double& third)
{
	double r = first * 20 / 51; // convert 0-255 to 0-100
	double g = second * 20 / 51;
	double b = third * 20 / 51;

	double max = std::max(std::max(r, g), b);
	double d = max - std::min(std::min(r, g), b);

	// hue
	if (d == 0)
	{
		// if r, g, and b are equal, set the hue to 0
		// to prevent dividing by 0
		first = 0;
	}
	else if (max == r)
	{
		first = std::fmod((60 * ((g - b) / d) + 360), 360);
	}
	else if (max == g)
	{
		first = std::fmod((60 * ((b - r) / d) + 120), 360);
	}
	else
	{
		first = std::fmod((60 * ((r - g) / d) + 240), 360);
	}

	// saturation
	if (max == 0)
	{
		second = 0;
	}
	else
	{
		second = (d / max) * 100;
	}

	third = max; // value
}

void hsv2rgb(double& first, double& second, double& third)
{
	double c = second * third / 10000;
	double x = c * (1 - std::abs(std::fmod((first / 60), 2) - 1));
	double m = third / 100 - c;

	if (first < 60)
	{
		first = (c + m) * 255; // r
		second = (x + m) * 255; // g
		third = m * 255; // b
	}
	else if (first < 120)
	{
		first = (x + m) * 255;
		second = (c + m) * 255;
		third = m * 255;
	}
	else if (first < 180)
	{
		first = m * 255;
		second = (c + m) * 255;
		third = (x + m) * 255;
	}
	else if (first < 240)
	{
		first = m * 255;
		second = (x + m) * 255;
		third = (c + m) * 255;
	}
	else if (first < 300)
	{
		first = (x + m) * 255;
		second = m * 255;
		third = (c + m) * 255;
	}
	else
	{
		first = (c + m) * 255;
		second = m * 255;
		third = (x + m) * 255;
	}
}

// convert black to transparent
void convert(std::vector<unsigned char>& image, unsigned int w, unsigned int h)
{
	for (long long i = 0; i < (w * 4) / 3; i += 4)
	{
		for (long long j = 0; j < h / 2; j++)
		{
			// if completely opaque
			if (image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 3] == 255)
			{
				double first = image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i];
				double second = image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 1];
				double third = image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 2];
				double alpha;
				rgb2hsv(first, second, third);
				alpha = third * 51 / 20; // convert 0-100 to 0-255
				third = 100;
				hsv2rgb(first, second, third);
				image[((w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i)] = first;
				image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 1] = second;
				image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 2] = third;
				image[(w * 4) / 3 * h / 2 - (j + 1) * (w * 4) / 3 + i + 3] = alpha;
			}
		}
	}
}

void fsbpng(std::string& folder, std::string& path, std::string output, bool& zip, std::filesystem::directory_entry png)
{
	out(1) << "FSB: Converting " + png.path().filename().u8string() << std::endl;
	int error;
	unsigned int w, h;
	std::vector<unsigned char> buffer, image, image1, image2, image3, top; // before h/2: bottom (rotate 90 counterclockwise), top (rotate 90 clockwise), south; h/2 to h: west, north, east
	// rotation: w*h - w + 1, w*h - 2*w + 1, ..., w*h - h*w + 1, w*h - w + 2, w*h - 2*w + 2, ..., w*h - w + w, w*h - 2*w + w, ...
	std::string filename = png.path().filename().u8string();
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	filename.erase(filename.end() - 4, filename.end());
	error = lodepng::load_file(buffer, png.path().u8string());
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::decode(image, w, h, state, buffer);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	if (w % 3 || h % 2)
	{
		out(5) << "FSB: Wrong dimensions: " << png.path().u8string();
		return;
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

	convert(image1, w, h);
	convert(image2, w, h);
	convert(image3, w, h);

	top.reserve(image.size() / 6);
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
	std::filesystem::create_directories(std::filesystem::u8path(zip ? "mcpppp-temp/" + folder + output : path + output));
	error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_bottom.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	buffer.clear();
	error = lodepng::encode(buffer, top, h / 2, w / 3, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_top.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	buffer.clear();
	error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_south.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
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

	convert(image1, w, h);
	convert(image2, w, h);
	convert(image3, w, h);

	buffer.clear();
	error = lodepng::encode(buffer, image1, w / 3, h / 2, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_west.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	buffer.clear();
	error = lodepng::encode(buffer, image2, w / 3, h / 2, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_north.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	buffer.clear();
	error = lodepng::encode(buffer, image3, w / 3, h / 2, state);
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
	error = lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + output + filename + "_east.png");
	if (error)
	{
		out(5) << "FSB: png error: " << lodepng_error_text(error) << std::endl;
	}
}

void fsbprop(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	int startfadein = -1, endfadein = -1, startfadeout = -1, endfadeout = -1;
	std::string name = png.path().filename().u8string(), source, option, value, temp;
	std::vector<unsigned char> buffer;
	lodepng::State state;
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	name.erase(name.end() - 11, name.end());
	source = name;
	std::stringstream ss;
	nlohmann::json j = { {"schemaVersion", 2}, {"type", "square-textured"}, {"conditions", {{"worlds", {"minecraft:overworld"}}}}, {"blend", true}, {"properties", {{"blend", {{"type", "add"}}}, {"rotation", {{"axis", {0.0, 180.0, 0.0}}}}, {"sunSkyTint", false}} } };
	std::ifstream fin(png.path());
	while (fin)
	{
		std::getline(fin, temp);
		option.clear();
		value.clear();
		bool isvalue = false;
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
		while (option.back() == ' ' || option.back() == '\t')
		{
			option.pop_back();
		}
		while (value.front() == ' ' || value.front() == '\t')
		{
			value.erase(value.begin());
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
			// apparently \: is valid syntax
			findreplace(temp, "\\:", ":");
			for (size_t i = 0; i < temp.size(); i++)
			{
				if (temp[i] == ':')
				{
					temp.erase(temp.begin() + i);
					i--;
				}
			}
			temp += '0';
			try
			{
				int tempi = stoi(temp);
				tempi = int(tempi / 1000) * 1000 + round((tempi % 1000) / 3.0 * 5);
				tempi = (tempi + 18000) % 24000;
				((option == "startFadeIn" || option == "startFadeOut") ? (option == "startFadeIn" ? startfadein : startfadeout) : (option == "endFadeIn" ? endfadein : endfadeout)) = tempi;
				j["properties"]["fade"][option] = tempi;
			}
			catch (std::invalid_argument& e)
			{
				out(5) << "Error: " << e.what() << "\n\tIn file \"" << png.path().u8string() << "\"\n\t" << "stoi argument is \"" << temp << "\"" << std::endl;
				return;
			}
		}
		else if (option == "blend")
		{
			j["properties"]["blend"]["type"] = value;
		}
		else if (option == "rotate")
		{
			j["properties"]["shouldRotate"] = (value == "true") ? true : false;
		}
		else if (option == "speed")
		{
			j["properties"]["rotation"]["rotationSpeed"] = stod(value);
		}
		else if (option == "axis")
		{
			std::string x, y, z;
			std::stringstream axis;
			axis.str(value);
			axis >> x >> y >> z;
			j["properties"]["rotation"]["axis"] = { stod(x) * 180, stod(y) * 180, stod(z) * 180 };
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
			j["conditions"]["biomes"] = biomelist;
		}
		if (option == "heights")
		{
			std::string height, minheight;
			std::stringstream heights;
			std::vector<nlohmann::json> heightlist;
			heights.str(value);
			while (heights)
			{
				heights >> height;
				for (size_t i = 0; i < height.size(); i++)
				{
					if (height[i] == '-')
					{
						minheight = height;
						minheight.erase(minheight.begin() + i, minheight.end());
						height.erase(height.begin(), height.begin() + i);
						heightlist.push_back({ {"min", stod(minheight)}, {"max", stod(height)} });
					}
				}
			}
			j["conditions"]["heights"] = heightlist;
		}
		if (option == "transition")
		{
			// dunno how this works either lol (will be changed when new ver of fsb is released maybe)
		}
	}
	fin.close();
	j["properties"]["rotation"]["static"] = { 1, 1, 1 };
	if (startfadeout == -1)
	{
		j["properties"]["fade"]["startFadeOut"] = (endfadeout - endfadein + startfadein + 24000) % 24000;
	}
	if (source[0] == '.' && source[1] == '/')
	{
		source.erase(source.begin());
		std::string origsource = source;
		temp = png.path().parent_path().u8string();
		if (temp.back() == '/' || temp.back() == '\\')
		{
			temp.erase(temp.end() - 1);
		}
		temp += source;
		source = "fabricskyboxes:sky" + source;
		std::filesystem::directory_entry entry = std::filesystem::directory_entry(std::filesystem::u8path(temp + ".png"));
		if (entry.exists())
		{
			fsbpng(folder, path, "/assets/fabricskyboxes/sky/", zip, entry);
		}
		else
		{
			out(4) << "FSB: File not found: " << temp + ".png" << std::endl;
			lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_top.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_bottom.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_north.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_south.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_west.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + origsource + "_east.png");
			buffer.clear();
			buffer.shrink_to_fit();
		}
	}
	else
	{
		std::string sourcefolder = source, sourcefile;
		while (sourcefolder.back() != '/' && sourcefolder.size() > 0)
		{
			sourcefolder.erase(sourcefolder.end() - 1);
		}
		sourcefile = std::string(source.begin() + sourcefolder.size(), source.end());
		if (sourcefolder.front() != '/')
		{
			sourcefolder.insert(sourcefolder.begin(), '/');
		}
		std::filesystem::directory_entry entry = std::filesystem::directory_entry(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + (source.front() == '/' ? "" : "/") + source + ".png"));
		if (entry.exists())
		{
			fsbpng(folder, path, "/assets/fabricskyboxes/sky" + sourcefolder, zip, entry);
		}
		else
		{
			out(4) << "FSB: File not found: " << sourcefolder + sourcefile + ".png" << std::endl;
			lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_top.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_bottom.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_north.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_south.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_west.png");
			lodepng::save_file(buffer, (zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky" + sourcefolder + sourcefile + "_east.png");
			buffer.clear();
			buffer.shrink_to_fit();
		}
		source = "fabricskyboxes:sky" + sourcefolder + sourcefile;
	}
	j["textures"]["top"] = source + "_top.png";
	j["textures"]["bottom"] = source + "_bottom.png";
	j["textures"]["north"] = source + "_north.png";
	j["textures"]["south"] = source + "_south.png";
	j["textures"]["west"] = source + "_west.png";
	j["textures"]["east"] = source + "_east.png";
	if (!std::filesystem::exists(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/")))
	{
		std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky"));
	}
	std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/fabricskyboxes/sky/" + name + ".json"));
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
			if (autoreconvert)
			{
				out(3) << "FSB: Reconverting " << filename << std::endl;
				zipa.DeleteEntry("assets/fabricskyboxes/");
			}
			else
			{
				out(2) << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
				return;
			}
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
			out(2) << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		out(3) << "FSB: Extracting " << filename << std::endl;
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/fabricskyboxes/sky")))
		{
			if (autoreconvert)
			{
				out(3) << "FSB: Reconverting " << filename << std::endl;
				std::filesystem::remove_all(std::filesystem::u8path(path + "/assets/fabricskyboxes"));
			}
			else
			{
				out(2) << "FSB: Fabricskyboxes folder found in " << filename << ", skipping" << std::endl;
				return;
			}
		}
		if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/optifine/sky")))
		{
			optifine = true;
		}
		else if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/mcpatcher/sky")))
		{
			optifine = false;
		}
		else
		{
			out(2) << "FSB: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		out(3) << "FSB: Converting Pack " << filename << std::endl;
	}
	for (auto& png : std::filesystem::directory_iterator(std::filesystem::u8path(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0/" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/sky/world0")))
	{
		if (png.path().extension() == ".properties")
		{
			out(1) << "FSB: Converting " + png.path().filename().u8string() << std::endl;
			fsbprop(folder, path, zip, png);
		}
	}
	if (zip)
	{
		out(3) << "FSB: Compressing " + filename << std::endl;
		Zippy::ZipEntryData zed;
		long long filesize;
		for (auto& png : std::filesystem::directory_iterator("mcpppp-temp/" + folder + "/assets/fabricskyboxes/sky"))
		{
			if (png.is_directory())
			{
				continue;
			}
			std::ifstream fin(png.path(), std::ios::binary | std::ios::ate);
			zed.clear();
			filesize = png.file_size();
			zed.resize(filesize);
			fin.seekg(0, std::ios::beg);
			fin.read((char*)(zed.data()), filesize);
			fin.close();
			zipa.AddEntry("assets/fabricskyboxes/sky/" + png.path().filename().u8string(), zed);
		}
		zed.clear();
		zed.shrink_to_fit();
		zipa.Save();
	}
	zipa.Close();
	std::filesystem::remove_all("mcpppp-temp");
}
