/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pch.h"

#include "convert.h"
#include "utility.h"

using mcpppp::output;
using mcpppp::level_t;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;
using mcpppp::checkpoint;

namespace fsb
{
	// check and handle lodepng error
	// @param i  error code
	static void checkError(const unsigned int i)
	{
		if (i != 0)
		{
			output<level_t::error>("FSB: png error: {}", lodepng_error_text(i));
		}
		checkpoint();
	}

	// convert optifine image format (1 image for all 6 sides) into fsb image format (1 image per side)
	// @param path  path of resourcepack
	// @param overworldsky  whether image is of overworld sky (and not end)
	// @param output  location to output to (relative to `path`)
	// @param entry  directory entry of image file to convert
	// @param filename  name of image file (no extension)
	static void png(const std::filesystem::path& path, const bool overworldsky, const std::u8string& output_path, const std::filesystem::directory_entry& entry, const std::u8string& filename)
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
		unsigned int w = 0, h = 0;
		std::vector<uint8_t> buffer, image, image1, image2, image3; // before h/2: bottom (rotate 90 counterclockwise), top, south; h/2 to h: west, north, east
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
		checkpoint();

		buffer.clear();
		std::filesystem::create_directories((path / output_path / filename).parent_path());
		checkpoint();

		// always output as 8-bit rgba
		state.info_png.color.colortype = LCT_RGBA;
		state.info_png.color.bitdepth = 8;
		state.encoder.auto_convert = false;

		checkError(lodepng::encode(buffer, image1, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image2, outh, outw / 4, state));
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
		checkpoint();

		buffer.clear();
		checkError(lodepng::encode(buffer, image1, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image2, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		buffer.clear();
		checkError(lodepng::encode(buffer, image3, outw / 4, outh, state));
		checkError(lodepng::save_file(buffer, c8tomb((path / output_path / (filename + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string())));
		checkpoint();
	}

	// convert optifine properties files into fsb properties json
	// @param path  path of resourcepack
	// @param optifine  true if in optifine directory, false if mcpatcher
	// @param overworldsky  whether properties file is overworld sky (and not end)
	// @param entry  directory entry of properties file
	static void prop(const std::filesystem::path& path, const bool overworldsky, const bool optifine, const std::filesystem::directory_entry& entry)
	{
		int startfadein = -1, endfadein = -1, startfadeout = -1, endfadeout = -1;
		const std::u8string name = entry.path().stem().generic_u8string();
		std::u8string source, u8temp;
		std::vector<uint8_t> buffer;
		lodepng::State state;
		state.info_raw.colortype = LCT_RGBA;
		state.info_raw.bitdepth = 8;
		source = u8"./" + name;
		bool reverse_speed = false;

		nlohmann::json j =
		{
			{"schemaVersion", 2},
			{"type", "square-textured"},
			{"conditions",
			{
				{"worlds", {(overworldsky ? "minecraft:overworld" : "minecraft:the_end")}},
				{"weather", {"clear"}}
			} },
			{"blend", {{"type", "add"}}},
			{"decorations",
			{
				{"showStars", false}, // stars are off in optifine skyboxes
				{"showSun", overworldsky}, // show sun/moon only in overworld
				{"showMoon", overworldsky},
			} },
			{"properties",
			{
				{"rotation",
				{
					{"axis", {90.0, 0.0, 0.0}}, // fsb uses {z, y, x} (kinda, see more explaination in `else if (option == "axis")`)
					{"static", {0.0, 0.0, 0.0}},
					{"rotationSpeed", 1.0}
				}}
			} }
		};

		std::ifstream fin(entry.path());
		std::string rawdata{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
		const auto prop_data = mcpppp::conv::parse_properties(rawdata);
		checkpoint();

		for (const auto& [option, value] : prop_data)
		{
			if (option == "source")
			{
				source = mbtoc8(value);
				if (source.ends_with(u8".png"))
				{
					source.erase(source.end() - 4, source.end());
				}
				checkpoint();
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
				checkpoint();
			}
			else if (option == "blend")
			{
				j["blend"]["type"] = value;
				checkpoint();
			}
			else if (option == "rotate")
			{
				j["properties"]["shouldRotate"] = (value == "true");
				checkpoint();
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
				checkpoint();
			}
			else if (option == "axis")
			{
				std::string x, y, z;
				std::istringstream axis(value);
				axis >> x >> y >> z;
				try
				{
					// fsb rotation is... kinda weird
					// first number is z axis, 90 = +z cw, -90 = -z cw
					// second number is y axis, 90 and -90 are both +y cw
					// third number is x axis, 90 = +x ccw, -90 = -x ccw (or 90 = -x cw, -90 = +x cw)
					double x_rotation = stod(x) * 90;
					double y_rotation = stod(y) * 90;
					double z_rotation = stod(z) * 90;

					// flip everything if y axis rotation is negative
					if (y_rotation < 0)
					{
						x_rotation = -x_rotation;
						z_rotation = -z_rotation;
						reverse_speed = true;
					}
					else
					{
						reverse_speed = false;
					}

					// reverse x axis rotation
					j["properties"]["rotation"]["axis"] = { z_rotation, y_rotation, -x_rotation };
				}
				catch (const std::invalid_argument& e)
				{
					output<level_t::error>("FSB Error: {}\n\tIn file \"{}\"\n\tstod argument is \"{}\"", e.what(), c8tomb(entry.path().generic_u8string()), value);
					return;
				}
				checkpoint();
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
				checkpoint();
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
				checkpoint();
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
					if (!height.empty())
					{
						const auto p = mcpppp::conv::parse_range(height);
						heightlist.push_back(nlohmann::json{ { {"min", p.first}, {"max", p.second} } });
					}
				}
				j["conditions"]["heights"] = heightlist;
				checkpoint();
			}
			else if (option == "transition")
			{
				// dunno how this works either lol (will be changed when new ver of fsb is released maybe)
			}
		}
		fin.close();
		checkpoint();

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
			// fill in missing startfadeout
			j["properties"]["fade"]["startFadeOut"] = (endfadeout - endfadein + startfadein + 24000) % 24000;
		}

		// reverse rotation direction if necessary
		if (reverse_speed)
		{
			j["properties"]["rotation"]["rotationSpeed"] = -j["properties"]["rotation"]["rotationSpeed"].get<double>();
		}


		std::filesystem::path base = path / "assets/minecraft";
		bool relative = false;
		std::u8string sourcefolder = u8"minecraft/", sourcefile;

		// set base path to properties directory
		if (source.starts_with(u8"./"))
		{
			source.erase(0, 2);
			base = entry.path().parent_path();
			sourcefolder.clear();
			relative = true;
		}
		// set base path to optifine/mcpatcher directory
		else if (source.starts_with(u8"~/"))
		{
			source.erase(0, 2);
			std::u8string_view str = (optifine ? u8"optifine/" : u8"mcpatcher/");
			sourcefolder += str;
			base /= str;
		}
		// set base path to namespace
		else if (source.find(u8':') != std::string::npos)
		{
			// extract namespace
			std::u8string ns;
			size_t pos = source.find(u8':');
			ns = { source.begin(), source.begin() + pos };
			source.erase(source.begin(), source.begin() + pos + 1);

			if (ns.find(u8'/') != std::string::npos)
			{
				throw std::runtime_error("FSB: Invalid namespace " + c8tomb(ns));
			}

			base = path / (u8"assets/" + ns);
			sourcefolder = ns + u8'/';
		}

		sourcefile = source;
		if (!relative)
		{
			size_t pos = source.rfind('/');
			if (pos != std::string::npos)
			{
				sourcefolder += { source.begin(), source.begin() + pos + 1 };
				// source without sourcefolder
				sourcefile = std::u8string(source.begin() + pos + 1, source.end());
			}
		}

		const std::filesystem::path image_noext = base / source;
		const std::filesystem::directory_entry image = std::filesystem::directory_entry(std::filesystem::path(image_noext).replace_extension(".png"));
		if (image.exists())
		{
			png(path, overworldsky, u8"assets/fabricskyboxes/sky/" + sourcefolder, image, image_noext.filename().u8string());
		}
		else
		{
			std::filesystem::path fsb_save_dir = path / "assets/fabricskyboxes/sky";
			if (!relative)
			{
				fsb_save_dir /= sourcefolder;
			}
			output<level_t::info>("(warn) FSB: File not found: {}.png", c8tomb(sourcefolder + sourcefile));
			std::filesystem::create_directories(fsb_save_dir);
			lodepng::encode(buffer, { 0, 0, 0, 1 }, 1, 1, state);
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_top" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_bottom" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_north" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_south" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_west" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			lodepng::save_file(buffer, c8tomb((fsb_save_dir / (sourcefile + u8"_east" + (overworldsky ? u8"" : u8"_end") + u8".png")).generic_u8string()));
			buffer.clear();
			buffer.shrink_to_fit();
			checkpoint();
		}
		source = u8"fabricskyboxes:sky/" + sourcefolder + sourcefile;

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
		checkpoint();
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool zip) noexcept
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
		checkpoint();
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
					prop(path, true, info.optifine, entry);
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
					prop(path, false, info.optifine, entry);
				}
			}
		}
	}
}
