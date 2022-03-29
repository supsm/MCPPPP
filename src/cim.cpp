/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <filesystem>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "convert.h"
#include "utility.h"

using mcpppp::out;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

namespace cim
{
	// convert non-properties (models and textures) to cim
	// @param path  path of resourcepack
	// @param zip  whether resourcepack is zipped
	// @param entry  directory entry of model/texture file
	static void other(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry)
	{
		// png location (textures): assets/mcpppp_hash/textures/item
		// json location (models): assets/mcpppp_hash/models/item
		// mcpppp_hash:item/

		const std::string mcnamespace = "mcpppp_" + mcpppp::convert::getfilenamehash(path, zip);
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		mcpppp::findreplace(folderpath, u8" ", u8"_");
		if (entry.path().extension() == ".png")
		{
			std::filesystem::create_directories(path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath);
			std::u8string filename = entry.path().filename().u8string();
			mcpppp::findreplace(filename, u8" ", u8"_");
			mcpppp::copy(entry.path(), path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath / filename);
		}
		else
		{
			std::ifstream fin(entry.path());
			nlohmann::ordered_json j;
			std::string temp;
			fin >> j;
			fin.close();
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
								mcpppp::findreplace(temp, " ", "_");
								// has no namespace, use default minecraft namespace
								mcpppp::copy(path / u8"assets/minecraft/textures" / (mbtoc8(origtemp) + u8".png"),
									path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra/minecraft" / (mbtoc8(temp) + u8".png"));
								it.value() = mcnamespace + ":extra/minecraft/" + temp;
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
								mcpppp::findreplace(temp, " ", "_");
								mcpppp::copy(path / u8"assets" / mbtoc8(ns) / u8"textures" / (mbtoc8(origtemp) + u8".png"),
									path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra" / mbtoc8(ns) / (mbtoc8(temp) + u8".png"));
								it.value() = mcnamespace + ":extra/" + ns + '/' + temp;
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
			std::filesystem::create_directories(path / u8"assets" / mbtoc8(mcnamespace) / "models/item" / folderpath);
			std::u8string filename = entry.path().filename().u8string();
			mcpppp::findreplace(filename, u8" ", u8"_");
			std::ofstream fout(path / u8"assets" / mbtoc8(mcnamespace) / "models/item" / folderpath / filename);
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
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
	static void read_prop(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry,
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
		const std::string mcnamespace = "mcpppp_" + mcpppp::convert::getfilenamehash(path, zip);
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		mcpppp::findreplace(folderpath, u8" ", u8"_");

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
				return std::string("<=").append(optifine_range.begin() + 1, optifine_range.end());
			}
			else if (optifine_range.ends_with('-'))
			{
				// range from number to anything (>=)
				return std::string(">=").append(optifine_range.begin(), optifine_range.end() - 1);
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
						return std::string(first).append("..").append(optifine_range.begin() + i + 1, optifine_range.end());
					}
				}
			}
		};

		std::ifstream fin(entry.path());
		const int filesize = std::filesystem::file_size(entry.path());
		std::string rawdata(filesize, 0);
		fin.read(rawdata.data(), filesize);
		fin.close();
		const auto prop_data = mcpppp::convert::parse_properties(rawdata);

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
					// remove minecraft: namespace
					if (temp.starts_with("minecraft:"))
					{
						temp.erase(temp.begin(), temp.begin() + 10);
					}
					items.push_back(temp);
				}
			}
			else if (option == "texture")
			{
				texture = value;
				mcpppp::findreplace(texture, " ", "_");
				// std::string::contains in C++23
				if (texture.find(".png") != std::string::npos)
				{
					texture.erase(texture.end() - 4, texture.end());
				}
				// std::string::contains in C++23
				if (texture.find('/') != std::string::npos && !texture.starts_with('.'))
				{
					// assets/mcpppp/textures/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra" / (mbtoc8(texture) + u8".png"));
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
			}
			else if (option.starts_with("texture."))
			{
				// TODO: texture.name (idk what this means)
			}
			else if (option == "model")
			{
				model = value;
				mcpppp::findreplace(model, " ", "_");
				// std::string::contains in C++23
				if (model.find(".json") != std::string::npos)
				{
					model.erase(model.end() - 5, model.end());
				}
				// std::string::contains in C++23
				if (model.find('/') != std::string::npos && !model.starts_with('.'))
				{
					// assets/mcpppp/models/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), path / u8"assets" / mbtoc8(mcnamespace) / "models/extra" / (mbtoc8(model) + u8".json"));
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
			}
			else if (option == "damage")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					damages.push_back(handlerange(temp));
				}
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
					stacksizes.push_back(handlerange(temp));
				}
			}
			else if (option == "enchantments")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					// std::string::contains in C++23
					// no namespace, insert default minecraft namespace instead
					if (temp.find(':') == std::string::npos)
					{
						temp.insert(0, "minecraft:");
					}
					enchantments.push_back(temp);
				}
			}
			else if (option == "enchantmentLevels")
			{
				std::istringstream ss(value);
				while (ss)
				{
					std::string temp;
					ss >> temp;
					enchantmentlevels.push_back(handlerange(temp));
				}
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
					temp = std::string("/").append(insensitive ? "(?i)" : "").append(mcpppp::convert::oftoregex(temp)).append("/");
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
			}
		}
	}

	// converts optifine cit properties to cim
	// @param path  path to resource pack
	// @param entry  directory entry of properties file
	static void prop(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry)
	{
		const std::string mcnamespace = "mcpppp_" + mcpppp::convert::getfilenamehash(path, zip);
		std::string type = "item", texture, model, hand = "anything", name;
		std::vector<std::string> items, enchantments, damages, stacksizes, enchantmentlevels;
		std::vector<nlohmann::json> nbts, predicates, tempp;
		std::stack<std::string> nbt;
		nlohmann::json tempj;

		read_prop(path, zip, entry, type, items, texture, model, damages, stacksizes, enchantments, enchantmentlevels, hand, nbts, name);

		if (type != "item") // TODO: add armor later
		{
			return;
		}

		predicates.clear();

		// create temporary model which points to texture if model is not supplied
		if (model.empty() && !texture.empty())
		{
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
		}
		tempj = { {"model", model} };

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
					tempj["predicate"]["damage"] = s;
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
		}
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool zip)
	{
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
	}

	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		out(3) << "CIM: Converting Pack " << c8tomb(filename) << std::endl;

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

		// convert non-prop files first, so they won't be missing when copying
		for (const auto& entry : otherfiles)
		{
			out(1) << "CIM: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			other(path, info.iszip, entry);
		}

		for (const auto& entry : propfiles)
		{
			out(1) << "CIM: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			prop(path, info.iszip, entry);
		}
	}
}
