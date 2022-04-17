/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "convert.h"
#include "utility.h"

#include "lodepng.h"
#include "json.hpp"

using mcpppp::output;
using mcpppp::level_t;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

namespace fsb
{
	// compare if two values are equal using epsilon
	template<class T>
	static bool compare(T first, T second) noexcept
	{
		return std::abs(first - second) < std::numeric_limits<T>::epsilon();
	}

	// convert red-green-blue color to hue-saturation-value color
	// @param first  red (input, 0-255) and hue (output, 0-360)
	// @param second  green (input, 0-255) and saturation (output, 0-100)
	// @param third  blue (input, 0-255) and value (output, 0-100)
	static void rgb2hsv(double& first, double& second, double& third) noexcept
	{
		const double r = first * 20 / 51; // convert 0-255 to 0-100
		const double g = second * 20 / 51;
		const double b = third * 20 / 51;

		const double max = std::max(std::max(r, g), b);
		const double d = max - std::min(std::min(r, g), b);

		// hue
		if (compare(d, 0.0))
		{
			// if r, g, and b are equal, set the hue to 0
			// to prevent dividing by 0
			first = 0;
		}
		else if (compare(max, r))
		{
			first = std::fmod((60 * ((g - b) / d) + 360), 360);
		}
		else if (compare(max, g))
		{
			first = std::fmod((60 * ((b - r) / d) + 120), 360);
		}
		else
		{
			first = std::fmod((60 * ((r - g) / d) + 240), 360);
		}

		// saturation
		if (compare(max, 0.0))
		{
			second = 0;
		}
		else
		{
			second = (d / max) * 100;
		}

		third = max; // value
	}

	// convert hue-saturation-value color to red-green-blue color
	// @param first   hue (output, 0-360) and red (input, 0-255)
	// @param second  saturation (output, 0-100) and green (input, 0-255)
	// @param third  value (output, 0-100) and blue (input, 0-255)
	static void hsv2rgb(double& first, double& second, double& third) noexcept
	{
		const double c = second * third / 10000;
		const double x = c * (1 - std::abs(std::fmod((first / 60), 2) - 1));
		const double m = third / 100 - c;

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
	// @param image  image to convert
	// @param w  width of image in pixels
	// @param h  height of image in pixels
	static void convert(std::vector<uint8_t>& image, const unsigned int w, const unsigned int h, const bool allowtransparency)
	{
		// skip transparency conversion
		if (!mcpppp::fsbtransparent || !allowtransparency)
		{
			return;
		}
		for (long long i = 0; i < w; i += 4)
		{
			for (long long j = 0; j < h; j++)
			{
				// do transparency, if completely opaque
				if (image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 3)) == 255)
				{
					double first = image.at(static_cast<size_t>(w * h - (j + 1) * w + i));
					double second = image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 1));
					double third = image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 2));
					rgb2hsv(first, second, third);
					const double alpha = third * 51 / 20; // convert 0-100 to 0-255
					third = 100;
					hsv2rgb(first, second, third);
					image.at(static_cast<size_t>(w * h - (j + 1) * w + i)) = static_cast<uint8_t>(first);
					image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 1)) = static_cast<uint8_t>(second);
					image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 2)) = static_cast<uint8_t>(third);
					image.at(static_cast<size_t>(w * h - (j + 1) * w + i + 3)) = static_cast<uint8_t>(alpha);
				}
			}
		}
	}

	// check and handle lodepng error
	// @param i  error code
	static constexpr void checkError(const unsigned int i)
	{
		if (i != 0)
		{
			output<level_t::error>("FSB: png error: {}", lodepng_error_text(i));
		}
	}

	// convert optifine image format (1 image for all 6 sides) into fsb image format (1 image per side)
	// @param path  path of resourcepack
	// @param overworldsky  whether image is of overworld sky (and not end)
	// @param allowtransparency  allow overriding alpha values (e.g. does not work with burn, so it is false for burn)
	// @param output  location to output to (relative to `path`)
	// @param entry  directory entry of image file to convert
	// @param filename  name of image file (no extension)
	static void png(const std::filesystem::path& path, const bool overworldsky, const bool allowtransparency, const std::u8string& output_path, const std::filesystem::directory_entry& entry, const std::u8string& filename)
	{
		output<level_t::detail>("FSB: Converting {}", c8tomb(entry.path().generic_u8string()));
		// skip if already converted
		// TODO: might cause some issues with reconverting
		if (std::filesystem::exists(path / output_path / (filename + u8"_top" + (overworldsky ? u8"" : u8"_end") + u8".png")) &&
			std::filesystem::exists(path / output_path / (filename + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")) &&
			std::filesystem::exists(path / output_path / (filename + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")) &&
			std::filesystem::exists(path / output_path / (filename + u8"_south" + (overworldsky ? u8"" : u8"_end") + u8".png")) &&
			std::filesystem::exists(path / output_path / (filename + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")) &&
			std::filesystem::exists(path / output_path / (filename + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")))
		{
			output<level_t::detail>("FSB: {} already found, skipping reconversion", c8tomb(entry.path().filename().u8string()));
			return;
		}
		unsigned int w, h;
		std::vector<uint8_t> buffer, image, image1, image2, image3, top; // before h/2: bottom (rotate 90 counterclockwise), top (rotate 90 clockwise), south; h/2 to h: west, north, east
		// rotation: w*h - w + 1, w*h - 2*w + 1, ..., w*h - h*w + 1, w*h - w + 2, w*h - 2*w + 2, ..., w*h - w + w, w*h - 2*w + w, ...
		lodepng::State state;
		state.info_raw.colortype = LCT_RGBA;
		state.info_raw.bitdepth = 8;
		checkError(lodepng::load_file(buffer, c8tomb(entry.path().generic_u8string())));
		checkError(lodepng::decode(image, w, h, state, buffer));
		if (w % 3 != 0 || h % 2 != 0)
		{
			output<level_t::info>("(warn) FSB: Wrong dimensions: {}\nwill be cropped to proper dimensions", c8tomb(entry.path().generic_u8string()));
		}
		image1.reserve(buffer.size() / 6);
		image2.reserve(buffer.size() / 6);
		image3.reserve(buffer.size() / 6);
		const unsigned int outw = w / 3 * 4;
		const unsigned int outh = h / 2;
		for (size_t i = 0; i < (w * 4) * outh; i++)
		{
			if (i % (w * 4) < outw)
			{
				image1.push_back(image.at(i));
			}
			else if (i % (w * 4) < 2 * outw)
			{
				image2.push_back(image.at(i));
			}
			else if (i % (w * 4) < 3 * outw)
			{
				image3.push_back(image.at(i));
			}
		}

		convert(image1, outw, outh, allowtransparency);
		convert(image2, outw, outh, allowtransparency);
		convert(image3, outw, outh, allowtransparency);

		top.reserve(image.size() / 6);
		for (long long i = 0; i < outw; i += 4)
		{
			for (long long j = 0; j < outh; j++)
			{
				top.push_back(image2.at(static_cast<size_t>(outw * outh - (j + 1) * outw + i)));
				top.push_back(image2.at(static_cast<size_t>(outw * outh - (j + 1) * outw + i + 1)));
				top.push_back(image2.at(static_cast<size_t>(outw * outh - (j + 1) * outw + i + 2)));
				top.push_back(image2.at(static_cast<size_t>(outw * outh - (j + 1) * outw + i + 3)));
			}
		}
		buffer.clear();
		std::filesystem::create_directories((path / output_path / filename).parent_path());
		checkError(lodepng::encode(buffer, image1, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, top, outh, outw / 4, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_top" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image3, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_south" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		image1.clear();
		image2.clear();
		image3.clear();
		for (size_t i = (w * 4) * outh; i < (w * 4) * 2 * outh; i++)
		{
			if (i % (w * 4) < outw)
			{
				image1.push_back(image.at(i));
			}
			else if (i % (w * 4) < 2 * outw)
			{
				image2.push_back(image.at(i));
			}
			else if (i % (w * 4) < 3 * outw)
			{
				image3.push_back(image.at(i));
			}
		}

		convert(image1, outw, outh, allowtransparency);
		convert(image2, outw, outh, allowtransparency);
		convert(image3, outw, outh, allowtransparency);
		buffer.clear();
		checkError(lodepng::encode(buffer, image1, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image2, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image3, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
	}

	// convert optifine properties files into fsb properties json
	// @param path  path of resourcepack
	// @param overworldsky  whether properties file is overworld sky (and not end)
	// @param entry  directory entry of properties file
	static void prop(const std::filesystem::path& path, const bool overworldsky, const std::filesystem::directory_entry& entry)
	{
		int startfadein = -1, endfadein = -1, startfadeout = -1, endfadeout = -1;
		const std::u8string name = entry.path().stem().generic_u8string();
		bool allowtransparency = false;
		std::u8string source, u8temp;
		std::vector<uint8_t> buffer;
		lodepng::State state;
		state.info_raw.colortype = LCT_RGBA;
		state.info_raw.bitdepth = 8;
		source = name;
		nlohmann::json j =
		{
			{"schemaVersion", 2},
			{"type", "square-textured"},
			{"conditions",
			{
				{"worlds", {(overworldsky ? "minecraft:overworld" : "minecraft:the_end")}}
			} },
			{"blend", true},
			{"properties",
			{
				{"blend", {{"type", "add"}}},
				{"rotation",
				{
					{"axis", {0.0, -180.0, 0.0}},
					{"static", {1.0, 1.0, 1.0}},
					{"rotationSpeed", -1.0} // TODO: find a rotation speed
				}},
				{"showStars", false}
			} }
		};

		std::ifstream fin(entry.path());
		std::string rawdata{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
		const auto prop_data = mcpppp::conv::parse_properties(rawdata);

		for (const auto& [option, value] : prop_data)
		{
			if (option == "source")
			{
				source = mbtoc8(value);
				source.erase(source.size() - 4);
			}
			else if (option == "startFadeIn" || option == "startFadeOut" || option == "endFadeIn" || option == "endFadeOut")
			{
				std::string temp = value;
				for (size_t i = 0; i < temp.size(); i++)
				{
					if (temp.at(i) == ':') // time separator
					{
						temp.erase(temp.begin() + static_cast<std::string::difference_type>(i));
						i--;
					}
				}
				temp += '0';
				try
				{
					int tempi = stoi(temp);
					tempi = tempi / 1000 * 1000 + static_cast<int>(round((tempi % 1000) / 3.0 * 5));
					tempi = (tempi + 18000) % 24000;
					((option == "startFadeIn" || option == "startFadeOut") ?
						(option == "startFadeIn" ? startfadein : startfadeout) :
						(option == "endFadeIn" ? endfadein : endfadeout))
						= tempi;
					j["properties"]["fade"][option] = tempi;
				}
				catch (const std::invalid_argument& e)
				{
					output<level_t::error>("FSB Error: {}\n\tIn file \"{}\"\n\tstoi argument is \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), temp);
					return;
				}
			}
			else if (option == "blend")
			{
				j["properties"]["blend"]["type"] = value;
				// i think these should be good with overriding alpha
				if (value == "add" || value == "subtract" || value == "replace" || value == "overlay")
				{
					allowtransparency = true;
				}
			}
			else if (option == "rotate")
			{
				j["properties"]["shouldRotate"] = (value == "true");
			}
			else if (option == "speed")
			{
				try
				{
					j["properties"]["rotation"]["rotationSpeed"] = stod(value);
				}
				catch (const std::invalid_argument& e)
				{
					output<level_t::error>("FSB Error: {}\n\tIn file \"{}\"\n\tstod argument is \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), value);
					return;
				}
			}
			else if (option == "axis")
			{
				std::string x, y, z;
				std::istringstream axis(value);
				axis >> x >> y >> z;
				try
				{
					j["properties"]["rotation"]["axis"] = { stod(x) * 180, stod(y) * 180, stod(z) * 180 };
				}
				catch (const std::invalid_argument& e)
				{
					output<level_t::error>("FSB Error: {}\n\tIn file \"{}\"\n\tstod argument is \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), value);
					return;
				}
			}
			else if (option == "weather")
			{
				std::string weather;
				std::istringstream weathers(value);
				std::vector<std::string> weatherlist;
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
				std::istringstream biomes(value);
				std::vector<std::string> biomelist;
				while (biomes)
				{
					biomes >> biome;
					biomelist.push_back(biome);
				}
				j["conditions"]["biomes"] = biomelist;
			}
			else if (option == "heights")
			{
				std::string height, minheight;
				std::stringstream heights(value);
				std::vector<nlohmann::json> heightlist;
				while (heights)
				{
					height.clear();
					heights >> height;
					for (size_t i = 0; i < height.size(); i++)
					{
						if (height.at(i) == '-')
						{
							minheight = height;
							minheight.erase(minheight.begin() + static_cast<std::string::difference_type>(i), minheight.end());
							height.erase(height.begin(), height.begin() + static_cast<std::string::difference_type>(i) + 1);
							try
							{
								heightlist.push_back(nlohmann::json({ {"min", stod(minheight)}, {"max", stod(height)} }));
							}
							catch (const std::invalid_argument& e)
							{
								output<level_t::error>("FSB Error: {}\n\tIn file \"{}\"\n\tstod arguments are \"{}\", \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), minheight, height);
								return;
							}
						}
					}
				}
				j["conditions"]["heights"] = heightlist;
			}
			else if (option == "transition")
			{
				// dunno how this works either lol (will be changed when new ver of fsb is released maybe)
			}
		}
		fin.close();

		if (startfadein == -1 || endfadein == -1 || endfadeout == -1)
		{
			// no time specified, assume always active
			j["properties"]["fade"]["startFadeIn"] = 0;
			j["properties"]["fade"]["endFadeIn"] = 0;
			j["properties"]["fade"]["startFadeOut"] = 0;
			j["properties"]["fade"]["endFadeOut"] = 0;
			j["properties"]["fade"]["alwaysOn"] = true;
		}
		else if (startfadeout == -1)
		{
			j["properties"]["fade"]["startFadeOut"] = (endfadeout - endfadein + startfadein + 24000) % 24000;
		}

		if (source.starts_with(u8"./"))
		{
			source.erase(source.begin(), source.begin() + 2);
			const std::u8string origsource = source;
			u8temp = entry.path().parent_path().generic_u8string();
			if (u8temp.back() != '/')
			{
				u8temp += '/';
			}
			u8temp += source;
			source = u8"fabricskyboxes:sky/" + source;
			const std::filesystem::directory_entry image = std::filesystem::directory_entry(std::filesystem::path(u8temp + u8".png"));
			if (image.exists())
			{
				png(path, overworldsky, allowtransparency, u8"assets/fabricskyboxes/sky", image, origsource);
			}
			else
			{
				output<level_t::info>("(warn) FSB: File not found: {}.png", c8tomb(u8temp));
				lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_top" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_south" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / (origsource + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				buffer.clear();
				buffer.shrink_to_fit();
			}
		}
		else
		{
			std::u8string sourcefolder = source, sourcefile;
			while (sourcefolder.back() != '/')
			{
				sourcefolder.erase(sourcefolder.end() - 1);
				if (sourcefolder.empty())
				{
					throw std::out_of_range("FSB: source does not contain a /");
				}
			}
			sourcefile = std::u8string(source.begin() + static_cast<std::string::difference_type>(sourcefolder.size()), source.end());
			if (sourcefolder.front() != '/')
			{
				sourcefolder.insert(sourcefolder.begin(), '/');
			}
			const std::filesystem::path image_noext = path / source;
			const std::filesystem::directory_entry image = std::filesystem::directory_entry(std::filesystem::path(image_noext).replace_extension(".png"));
			if (image.exists())
			{
				png(path, overworldsky, allowtransparency, u8"assets/fabricskyboxes/sky" + sourcefolder, image, image_noext.filename().u8string());
			}
			else
			{

				output<level_t::info>("(warn) FSB: File not found: {}.png", c8tomb(sourcefolder + sourcefile));
				lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_top" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_south" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				lodepng::save_file(buffer, c8tomb((path / u8"assets/fabricskyboxes/sky" / sourcefolder / (sourcefile + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
				buffer.clear();
				buffer.shrink_to_fit();
			}
			source = u8"fabricskyboxes:sky" + sourcefolder + sourcefile;
		}

		j["textures"]["top"] = c8tomb(source) + "_top" + (overworldsky ? "" : "_end") + ".png";
		j["textures"]["bottom"] = c8tomb(source) + "_bottom" + (overworldsky ? "" : "_end") + ".png";
		j["textures"]["north"] = c8tomb(source) + "_north" + (overworldsky ? "" : "_end") + ".png";
		j["textures"]["south"] = c8tomb(source) + "_south" + (overworldsky ? "" : "_end") + ".png";
		j["textures"]["west"] = c8tomb(source) + "_west" + (overworldsky ? "" : "_end") + ".png";
		j["textures"]["east"] = c8tomb(source) + "_east" + (overworldsky ? "" : "_end") + ".png";

		if (!std::filesystem::exists(path / u8"assets/fabricskyboxes/sky/"))
		{
			std::filesystem::create_directories(path / u8"assets/fabricskyboxes/sky");
		}
		// add _end to filename if end sky (to prevent conflicts)
		std::ofstream fout(path / u8"assets/fabricskyboxes/sky/" / (name + (overworldsky ? u8"" : u8"_end") + u8".json"));
		fout << j.dump(1, '\t') << std::endl;
		fout.close();
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool zip)
	{
		using mcpppp::checkresults;

		bool reconverting = false;

		if (mcpppp::findfolder(path, u8"assets/fabricskyboxes/sky/", zip))
		{
			if (mcpppp::autoreconvert)
			{
				reconverting = true;
			}
			else
			{
				return { checkresults::alrfound, false, false, zip };
			}
		}
		if (mcpppp::findfolder(path, u8"assets/minecraft/optifine/sky/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, false, zip };
			}
			else
			{
				return { checkresults::valid, true, false, zip };
			}
		}
		else if (mcpppp::findfolder(path, u8"assets/minecraft/mcpatcher/sky/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, false, false, zip };
			}
			else
			{
				return { checkresults::valid, false, false, zip };
			}
		}
		else
		{
			// no convertible locations found
			return { checkresults::noneconvertible, false, false, zip };
		}
	}

	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		output<level_t::important>("FSB: Converting Pack {}", c8tomb(filename));
		// overworld sky (world0)
		const std::filesystem::path overworld = path / u8"assets/minecraft" / (info.optifine ? u8"optifine" : u8"mcpatcher") / u8"sky/world0";
		if (std::filesystem::exists(overworld))
		{
			for (const auto& entry : std::filesystem::directory_iterator(overworld))
			{
				if (entry.path().extension() == ".properties")
				{
					output<level_t::detail>("FSB: Converting {}", c8tomb(entry.path().generic_u8string()));
					prop(path, true, entry);
				}
			}
		}
		// end sky (world1)
		const std::filesystem::path end = path / u8"assets/minecraft" / (info.optifine ? u8"optifine" : u8"mcpatcher") / u8"sky/world1";
		if (std::filesystem::exists(end))
		{
			for (const auto& entry : std::filesystem::directory_iterator(end))
			{
				if (entry.path().extension() == ".properties")
				{
					output<level_t::detail>("FSB: Converting {}", c8tomb(entry.path().generic_u8string()));
					prop(path, false, entry);
				}
			}
		}
	}
}
