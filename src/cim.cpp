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

namespace cim
{
	const static std::regex bad_regex("[^a-z0-9/._-]", std::regex_constants::optimize | std::regex_constants::ECMAScript);

	static void fixpathchars(std::string& s)
	{
		mcpppp::findreplace(s, " ", "_");
		s = mcpppp::lowercase(s);
		s = std::regex_replace(s, bad_regex, "-");
		checkpoint();
	}

	static void fixpathchars(std::u8string& s)
	{
		mcpppp::findreplace(s, u8" ", u8"_");
		s = mcpppp::lowercase(s);
		s = mbtoc8(std::regex_replace(reinterpret_cast<const char*>(s.c_str()), bad_regex, "-"));
		checkpoint();
	}

	// change model paths to work with cim
	// @param path  path of resourcepack
	// @param mcnamespace  mcpppp namespace to use
	// @param folderpath  relative path of parent folder of `entry` from cim folder
	// @param outputpath  path of file to output to, relative to assets/`mcnamespace`/models
	// @param entry  directory entry of model file
	// @return whether the conversion succeeded
	static bool convert_model(const std::filesystem::path& path, const std::string& mcnamespace, const std::u8string& folderpath, const std::filesystem::path& outputpath, const std::filesystem::directory_entry& entry)
	{
		if (!entry.exists())
		{
			output<level_t::error>("CIM: Model not found: {}", c8tomb(entry.path().generic_u8string()));
			return false;
		}
		std::ifstream fin(entry.path());
		nlohmann::ordered_json j;
		std::string temp;
		try
		{
			fin >> j;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			output<level_t::error>("CIM: Json parse error in {}", c8tomb(entry.path().generic_u8string()));
			return false;
		}
		fin.close();
		checkpoint(); // finish reading file

		if (j.contains("parent"))
		{
			temp = j["parent"].get<std::string>();
			if (temp.starts_with("./"))
			{
				temp.erase(temp.begin(), temp.begin() + 2);
				temp = mcnamespace + ":item/" + c8tomb(folderpath) + temp;
				j["parent"] = temp;
			}
		}
		if (j.contains("textures"))
		{
			bool layer0 = false;
			std::string first;
			for (const auto& it : j["textures"].items())
			{
				if (it.value().type() == nlohmann::json::value_t::string)
				{
					if (it.key() == "layer0")
					{
						layer0 = true;
					}
					temp = it.value().get<std::string>();
					if (temp.starts_with("./"))
					{
						temp.erase(temp.begin(), temp.begin() + 2);
						temp.insert(0, mcnamespace + ":item/" + c8tomb(folderpath));
						it.value() = temp;
					}
					// std::string::contains in C++23
					if (temp.find(' ') != std::string::npos)
					{
						std::string origtemp;
						// std::string::contains in C++23
						if (temp.find(':') == std::string::npos)
						{
							// optifine allows spaces somehow
							origtemp = temp;
							fixpathchars(temp);
							// has no namespace, use default minecraft namespace
							if (!mcpppp::copy(path / u8"assets/minecraft/textures" / (mbtoc8(origtemp) + u8".png"),
								path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra/minecraft" / (mbtoc8(temp) + u8".png")))
							{
								return false;
							}
							it.value() = fmt::format("{}:extra/minecraft/{}", mcnamespace, temp);
						}
						else
						{
							// has a namespace, use it instead
							std::string ns;
							// remove the namespace
							for (size_t i = 0; i < temp.size(); i++)
							{
								if (temp.at(i) == ':')
								{
									temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i) + 1);
									break;
								}
								ns.push_back(temp.at(i));
							}
							origtemp = temp;
							fixpathchars(temp);
							if (!mcpppp::copy(path / u8"assets" / mbtoc8(ns) / u8"textures" / (mbtoc8(origtemp) + u8".png"),
								path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra" / mbtoc8(ns) / (mbtoc8(temp) + u8".png")))
							{
								return false;
							}
							it.value() = fmt::format("{}:extra/{}/{}", mcnamespace, ns, temp);
						}
					}
					if (first.empty())
					{
						first = it.value();
					}
				}
			}
			if (!layer0)
			{
				j["textures"]["layer0"] = first;
			}
		}
		checkpoint(); // finish parsing

		std::filesystem::create_directories(path / u8"assets" / mbtoc8(mcnamespace) / "models" / outputpath);
		std::u8string filename = entry.path().filename().u8string();
		fixpathchars(filename);
		std::ofstream fout(path / u8"assets" / mbtoc8(mcnamespace) / "models" / outputpath / filename);
		fout << j.dump(1, '\t') << std::endl;
		fout.close();
		checkpoint();
		return true;
	}

	// convert non-properties (models and textures) to cim
	// @param path  path of resourcepack
	// @param zip  whether resourcepack is zipped
	// @param entry  directory entry of model/texture file
	static void other(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry)
	{
		// png location (textures): assets/mcpppp_hash/textures/item
		// json location (models): assets/mcpppp_hash/models/item
		// mcpppp_hash:item/

		const std::string mcnamespace = "mcpppp_" + mcpppp::conv::getfilenamehash(path, zip);
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		fixpathchars(folderpath);
		if (entry.path().extension() == ".png")
		{
			std::filesystem::create_directories(path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath);
			std::u8string filename = entry.path().filename().u8string();
			fixpathchars(filename);
			mcpppp::copy(entry.path(), path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath / filename);
		}
		else
		{
			if (!convert_model(path, mcnamespace, folderpath, u8"item/" + folderpath, entry))
			{
				output<level_t::error>("CIM: Invalid model: {}", c8tomb(entry.path().u8string()));
			}
		}
		checkpoint();
	}

	// read and parse optifine properties file
	// @param path  path to resource pack
	// @param zip  whether resourcepack is zipped
	// @param entry  directory entry of properties file to parse
	// @param type  type of texture replacement (item, enchantment, armor, elytra) (output)
	// @param items  items to apply replacement to (output)
	// @param texture  replacement texture to use (output)
	// @param model  replacement model to use (output)
	// @param damages  damage predicates (output)
	// @param stacksizes  stack size predicates (output)
	// @param enchantments  enchantment predicates (output)
	// @param enchantmentlevels  enchantment levels for enchantment predicates (output)
	// @param hand  hand predicate (ouptut)
	// @param nbts  nbt predicate. nbt.fireworks.explosions would become { "nbt", "fireworks", "explosions" } (output)
	// @param name  name predicate (output)
	// @return whether the properties file is valid or has issues
	static bool read_prop(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry,
		std::string& type,
		std::vector<std::string>& items,
		std::string& texture,
		std::string& model,
		std::vector<std::string>& damages,
		std::vector<std::string>& stacksizes,
		std::vector<std::string>& enchantments,
		std::vector<std::string>& enchantmentlevels,
		std::string& hand,
		std::vector<nlohmann::json>& nbts,
		std::string& name)
	{
		const std::string mcnamespace = "mcpppp_" + mcpppp::conv::getfilenamehash(path, zip);
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		fixpathchars(folderpath);

		// convert range stuff from optifine format (with -) to chime format (with [,] >= <= etc)
		const auto handlerange = [](const std::string& optifine_range) -> std::string
		{
			if (optifine_range.find('-') == std::string::npos)
			{
				// no range
				return optifine_range;
			}
			else if (optifine_range.starts_with('-'))
			{
				// range from anything to number (<=)
				return fmt::format("<={}", std::string(optifine_range.begin() + 1, optifine_range.end()));
			}
			else if (optifine_range.ends_with('-'))
			{
				// range from number to anything (>=)
				return fmt::format(">={}", std::string(optifine_range.begin(), optifine_range.end() - 1));
			}
			else
			{
				// range between two numbers
				for (size_t i = 0; i < optifine_range.size(); i++)
				{
					if (optifine_range.at(i) == '-')
					{
						std::string first = optifine_range;
						first.erase(first.begin() + static_cast<std::string::difference_type>(i), first.end());
						return fmt::format("{}..{}", first, std::string(optifine_range.begin() + i + 1, optifine_range.end()));
					}
				}
			}
			checkpoint();
			return std::string();
		};

		std::ifstream fin(entry.path());
		std::string rawdata{ std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>() };
		fin.close();
		const auto prop_data = mcpppp::conv::parse_properties(rawdata);
		checkpoint(); // finish reading and parsing file

		for (const auto& [option, value] : prop_data)
		{
			if (option == "type")
			{
				type = value;
			}
			else if (option == "items" || option == "matchItems") // matchItems not documented but i found it in a pack
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp.empty())
					{
						continue;
					}
					// remove minecraft: namespace
					if (temp.starts_with("minecraft:"))
					{
						temp.erase(temp.begin(), temp.begin() + 10);
					}
					items.push_back(temp);
				}
				checkpoint();
			}
			else if (option == "texture")
			{
				texture = value;
				fixpathchars(texture);
				if (texture.ends_with(".png"))
				{
					texture.erase(texture.end() - 4, texture.end());
				}
				// std::string::contains in C++23
				// contains directory separator but does not start with ./
				if (texture.find('/') != std::string::npos && !texture.starts_with('.'))
				{
					// assets/mcpppp/textures/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					if (!mcpppp::copy(path / u8"assets/minecraft" / (mbtoc8(texture) + u8".png"), path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra" / (mbtoc8(texture) + u8".png")))
					{
						return false;
					}
					texture.insert(0, mcnamespace + ":extra/");
				}
				else
				{
					if (texture.starts_with("./"))
					{
						texture.erase(texture.begin(), texture.begin() + 2);
					}
					texture.insert(0, mcnamespace + ":item/" + c8tomb(folderpath));
				}
				checkpoint();
			}
			else if (option.starts_with("texture."))
			{
				// TODO: texture.name (idk what this means)
			}
			else if (option == "model")
			{
				model = value;
				fixpathchars(model);
				// std::string::contains in C++23
				if (model.find(".json") != std::string::npos)
				{
					model.erase(model.end() - 5, model.end());
				}
				// std::string::contains in C++23
				// contains directory separator but does not start with ./
				if (model.find('/') != std::string::npos && !model.starts_with('.'))
				{
					// assets/mcpppp/models/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					const std::u8string modelpath = std::filesystem::path(model).parent_path().generic_u8string();
					if (!convert_model(path, mcnamespace, modelpath, u8"extra/" + modelpath, std::filesystem::directory_entry(path / u8"assets/minecraft/models" / (mbtoc8(model) + u8".json"))))
					{
						return false;
					}
					model.insert(0, mcnamespace + ":extra/");
				}
				else
				{
					if (model.starts_with("./"))
					{
						model.erase(model.begin(), model.begin() + 2);
					}
					model.insert(0, mcnamespace + ":item/" + c8tomb(folderpath));
				}
				checkpoint();
			}
			else if (option == "damage")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp.empty())
					{
						continue;
					}
					damages.push_back(handlerange(temp));
				}
				checkpoint();
			}
			else if (option == "damageMask")
			{
				// TODO: find out what this means
			}
			else if (option == "stackSize")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp.empty())
					{
						continue;
					}
					stacksizes.push_back(handlerange(temp));
				}
				checkpoint();
			}
			else if (option == "enchantments")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp.empty())
					{
						continue;
					}
					// std::string::contains in C++23
					// no namespace, insert default minecraft namespace instead
					if (temp.find(':') == std::string::npos)
					{
						temp.insert(0, "minecraft:");
					}
					enchantments.push_back(temp);
				}
				checkpoint();
			}
			else if (option == "enchantmentLevels")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					if (temp.empty())
					{
						continue;
					}
					enchantmentlevels.push_back(handlerange(temp));
				}
				checkpoint();
			}
			else if (option == "hand")
			{
				if (value == "any")
				{
					hand = "either";
				}
				else
				{
					hand = value;
				}
				checkpoint();
			}
			else if (option.starts_with("nbt."))
			{
				std::string temp;
				std::stack<std::string> nbt;
				for (const char& c : option)
				{
					if (c == '.')
					{
						nbt.push(temp);
						temp.clear();
					}
					else
					{
						temp += c;
					}
				}
				nbt.push(temp);
				temp = value;

				// handle regex/optifine pattern
				if (value.starts_with("regex:") || value.starts_with("iregex:"))
				{
					// if first character is i, then it is iregex (case insensitive)
					const bool insensitive = (value.front() == 'i');
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = std::string("/").append(insensitive ? "(?i)" : "").append(temp).append("/");
				}
				else if (value.starts_with("pattern:") || value.starts_with("ipattern:"))
				{
					const bool insensitive = (value.front() == 'i');
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = std::string("/").append(insensitive ? "(?i)" : "").append(mcpppp::conv::oftoregex(temp)).append("/");
				}

				if (mcpppp::lowercase(option) == "nbt.display.name")
				{
					name = temp;
				}
				else
				{
					nlohmann::json tempj = { {nbt.top(), temp} };
					nbt.pop();
					while (!nbt.empty())
					{
						tempj = { {nbt.top(), tempj} };
						nbt.pop();
					}
					nbts.push_back(tempj);
				}
				checkpoint();
			}
		}
		checkpoint();
		return true;
	}

	// converts optifine cit properties to cim
	// @param path  path to resource pack
	// @param entry  directory entry of properties file
	static void prop(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry)
	{
		const std::string mcnamespace = "mcpppp_" + mcpppp::conv::getfilenamehash(path, zip);
		std::string type = "item", texture, model, hand = "anything", name;
		std::vector<std::string> items, enchantments, damages, stacksizes, enchantmentlevels;
		std::vector<nlohmann::json> nbts, predicates, tempp;
		std::stack<std::string> nbt;
		nlohmann::json tempj;

		if (!read_prop(path, zip, entry, type, items, texture, model, damages, stacksizes, enchantments, enchantmentlevels, hand, nbts, name))
		{
			// return if prop file has issues
			output<level_t::error>("CIM: One or more issues found in {}, skipping", c8tomb(entry.path().u8string()));
			return;
		}
		checkpoint(); // finish reading file

		if (type != "item") // TODO: add armor later
		{
			return;
		}

		predicates.clear();

		// create temporary model which points to texture if model is not supplied
		if (model.empty())
		{
			// texture and model are both empty, warn and skip
			if (texture.empty())
			{
				output<level_t::info>("(warn) CIM: Texture and Model are both empty in {}", c8tomb(entry.path().generic_u8string()));
				return;
			}
			model = texture;
			// replace namespace:item/ with namespace:temp_models/
			if (model.starts_with(mcnamespace + ":item/"))
			{
				// 6 is size of ":item/"
				model.replace(mcnamespace.size(), 6, ":temp_models/");
			}

			// model and texture should not contain file extensions, add it here
			// 13 is size of ":temp_models/"
			const std::filesystem::path modelpath = path / u8"assets" / mbtoc8(mcnamespace) /
				mbtoc8(std::string("models/temp_models/").append(model.begin() + mcnamespace.size() + 13, model.end()).append(".json"));

			if (!std::filesystem::exists(modelpath.parent_path()))
			{
				std::filesystem::create_directories(modelpath.parent_path());
			}
			std::ofstream fout(modelpath);
			fout << nlohmann::json({ {"parent", "minecraft:item/generated"}, {"textures", {{"layer0", texture}}} }).dump(1, '\t') << std::endl;
			checkpoint(); // finish creating model
		}
		tempj = { {"model", model} };

		// TODO: skip empty predicates?

		if (hand != "anything")
		{
			tempj["predicate"]["entity"]["hand"] = hand;
		}
		if (!name.empty())
		{
			tempj["predicate"]["name"] = name;
		}
		predicates.push_back(tempj);

		if (!nbts.empty())
		{
			tempp.clear();
			for (const nlohmann::json& j : predicates)
			{
				for (nlohmann::json& j2 : nbts)
				{
					tempj = j;
					tempj["predicate"]["nbt"] = j2["nbt"];
					tempp.push_back(tempj);
				}
			}
			predicates = tempp;
		}

		if (!enchantments.empty())
		{
			tempp.clear();
			for (const nlohmann::json& j : predicates)
			{
				for (std::string& s : enchantments)
				{
					tempj = j;
					tempj["predicate"]["nbt"]["Enchantments"].push_back(nlohmann::json({ {"id", s} }));
					tempp.push_back(tempj);
				}
			}
			predicates = tempp;
		}

		if (!enchantmentlevels.empty())
		{
			tempp.clear();
			for (const nlohmann::json& j : predicates)
			{
				for (std::string& s : enchantmentlevels)
				{
					tempj = j;
					if (tempj["predicate"]["nbt"]["Enchantments"].type() == nlohmann::json::value_t::array)
					{
						for (nlohmann::json& j2 : tempj["predicate"]["nbt"]["Enchantments"])
						{
							j2["lvl"] = s;
							tempp.push_back(tempj);
						}
					}
					else
					{
						tempj["predicate"]["nbt"]["Enchantments"].push_back(nlohmann::json{ {"lvl", s} });
						tempp.push_back(tempj);
					}
				}
			}
			predicates = tempp;
		}

		if (!damages.empty())
		{
			tempp.clear();
			for (const nlohmann::json& j : predicates)
			{
				for (std::string& s : damages)
				{
					tempj = j;
					tempj["predicate"]["durability"] = s;
					tempp.push_back(tempj);
				}
			}
			predicates = tempp;
		}

		if (!stacksizes.empty())
		{
			tempp.clear();
			for (const nlohmann::json& j : predicates)
			{
				for (std::string& s : stacksizes)
				{
					tempj = j;
					tempj["predicate"]["count"] = s;
					tempp.push_back(tempj);
				}
			}
			predicates = tempp;
		}
		checkpoint(); // finish adding predicates to json

		nlohmann::json j = { {"parent", "minecraft:item/generated"}, {"textures", {{"layer0", texture}}}, {"overrides", predicates} };
		for (const std::string& c : items)
		{
			std::filesystem::create_directories(path / u8"assets/minecraft/overrides/item");
			tempj = j;
			std::ifstream fin2(path / u8"assets/minecraft/overrides/item" / (mbtoc8(c) + u8".json"));
			if (fin2.good())
			{
				fin2 >> tempj;
				std::vector<nlohmann::json> tempv = tempj["overrides"];
				std::copy(predicates.begin(), predicates.end(), std::back_inserter(tempv));
				tempj["overrides"] = tempv;
			}
			fin2.close();
			std::ofstream fout(path / u8"assets/minecraft/overrides/item" / (mbtoc8(c) + u8".json"));
			fout << tempj.dump(1, '\t') << std::endl;
			fout.close();
			checkpoint(); // finish editing json file
		}
		checkpoint();
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool zip) noexcept
	{
		checkpoint();
		using mcpppp::checkresults;
		bool reconverting = false;
		if (mcpppp::findfolder(path, u8"assets/minecraft/overrides/", zip))
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
		if (mcpppp::findfolder(path, u8"assets/minecraft/optifine/cit/", zip))
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
		else if (mcpppp::findfolder(path, u8"assets/minecraft/mcpatcher/cit/", zip))
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
			return { checkresults::noneconvertible, false, false, zip };
		}
		checkpoint();
	}

	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		output<level_t::important>("CIM: Converting Pack {}", c8tomb(filename));

		std::vector<std::filesystem::directory_entry> otherfiles, propfiles;

		// save other files and prop files so we don't need to iterate over the directory twice
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			path / u8"assets/minecraft" / (info.optifine ? u8"optifine" : u8"mcpatcher") / u8"cit"))
		{
			if (entry.path().extension() == ".json" || entry.path().extension() == ".png")
			{
				otherfiles.push_back(entry);
			}
			else if (entry.path().extension() == ".properties")
			{
				propfiles.push_back(entry);
			}
		}
		checkpoint(); // finish adding stuff

		// convert non-prop files first, so they won't be missing when copying
		for (const auto& entry : otherfiles)
		{
			output<level_t::detail>("CIM: Converting {}", c8tomb(entry.path().generic_u8string()));
			other(path, info.iszip, entry);
			checkpoint(); // finish conversion
		}

		for (const auto& entry : propfiles)
		{
			output<level_t::detail>("CIM: Converting {}", c8tomb(entry.path().generic_u8string()));
			prop(path, info.iszip, entry);
			checkpoint(); // finish conversion
		}
		checkpoint();
	}
}
