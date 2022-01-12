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

namespace cim
{
	// converts non-properties (models and textures) to cim
	static void other(const std::string& path, const std::filesystem::directory_entry& entry)
	{
		// png location (textures): assets/mcpppp/textures/item
		// json location (models): assets/mcpppp/models/item
		// mcpppp:item/

		std::string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind("/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		if (entry.path().extension() == ".png")
		{
			std::filesystem::create_directories(std::filesystem::u8path(path + "/assets/mcpppp/textures/item/" + folderpath));
			std::string filename = entry.path().filename().u8string();
			mcpppp::findreplace(filename, " ", "_");
			copy(entry.path(), std::filesystem::u8path(path + "/assets/mcpppp/textures/item/" + folderpath + filename));
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
				if (temp.at(0) == '.' && temp.at(1) == '/')
				{
					temp.erase(temp.begin(), temp.begin() + 2);
					temp = "mcpppp:item/" + folderpath + temp;
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
						if (temp.at(0) == '.' && temp.at(1) == '/')
						{
							temp.erase(temp.begin(), temp.begin() + 2);
							temp.insert(0, "mcpppp:item/" + folderpath);
							it.value() = temp;
						}
						if (temp.find(' ') != std::string::npos)
						{
							std::string origtemp = temp;
							mcpppp::findreplace(temp, " ", "_");
							if (temp.find(':') == std::string::npos)
							{
								mcpppp::copy(std::filesystem::u8path(path + "/assets/minecraft/textures/" + origtemp + ".png"),
									std::filesystem::u8path(path + "/assets/mcpppp/textures/extra/minecraft/" + temp + ".png"));
								it.value() = "mcpppp:extra/minecraft/" + temp;
							}
							else
							{
								std::string ns;
								for (size_t i = 0; i < origtemp.size(); i++)
								{
									if (origtemp.at(i) == ':')
									{
										origtemp.erase(origtemp.begin(), origtemp.begin() + static_cast<std::string::difference_type>(i));
										break;
									}
									ns.push_back(origtemp.at(i));
								}
								mcpppp::copy(std::filesystem::u8path(path + "/assets/" + ns + "/textures/" + origtemp + ".png"),
									std::filesystem::u8path(path + "/assets/mcpppp/textures/extra/" + ns + "/" + temp + ".png"));
								it.value() = "mcpppp:extra/" + ns + "/" + temp;
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
			std::filesystem::create_directories(std::filesystem::u8path(path + "/assets/mcpppp/models/item/" + folderpath));
			std::string filename = entry.path().filename().u8string();
			mcpppp::findreplace(filename, " ", "_");
			std::ofstream fout(std::filesystem::u8path(path + "/assets/mcpppp/models/item/" + folderpath + filename));
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

	// converts cit properties to cim
	static void prop(const std::string& path, const std::filesystem::directory_entry& entry)
	{
		std::string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind("/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		std::string temp, option, value, type = "item", texture, model, hand = "anything", first, name;
		std::vector<std::string> items, enchantments, damages, stacksizes, enchantmentlevels;
		std::vector<nlohmann::json> nbts, predicates, tempp;
		std::stack<std::string> nbt;
		nlohmann::json tempj;
		std::ifstream fin(entry.path());
		while (fin)
		{
			getline(fin, temp);
			option.clear();
			value.clear();
			bool isvalue = false;
			if (temp.empty() || temp.at(0) == '#')
			{
				continue;
			}
			for (const char& c : temp)
			{
				if (c == '=')
				{
					isvalue = true;
				}
				else if (!isvalue)
				{
					option += c;
				}
				else // isvalue
				{
					value += c;
				}
			}
			if (option == "type")
			{
				type = value;
			}
			else if (option == "items" || option == "matchItems") // matchItems not documented but i found it in a pack
			{
				std::stringstream ss;
				ss.str(value);
				while (ss)
				{
					ss >> temp;
					if (temp.find("minecraft:") == 0)
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
				if (texture.find(".png") != std::string::npos)
				{
					texture.erase(texture.end() - 4, texture.end());
				}
				if (texture.find('/') != std::string::npos && texture.at(0) != '.')
				{
					// assets/mcpppp/textures/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), std::filesystem::u8path(path + "/assets/mcpppp/textures/extra/" + texture + ".png"));
					texture.insert(0, "mcpppp:extra/");
				}
				else if (texture.at(0) == '.' && texture.at(1) == '/')
				{
					texture.erase(texture.begin(), texture.begin() + 2);
					texture.insert(0, "mcpppp:item/" + folderpath);
				}
				else
				{
					texture = "mcpppp:item/" + texture;
				}
			}
			else if (option.find("texture.") == 0)
			{
				// TODO: texture.name (idk what this means)
			}
			else if (option == "model")
			{
				model = value;
				mcpppp::findreplace(model, " ", "_");
				if (model.find(".json") != std::string::npos)
				{
					model.erase(model.end() - 5, model.end());
				}
				if (model.find('/') != std::string::npos && model.at(0) != '.')
				{
					// assets/mcpppp/models/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), std::filesystem::u8path(path + "/assets/mcpppp/models/extra/" + model + ".png"));
					model.insert(0, "mcpppp:extra/");
				}
				else if (model.at(0) == '.' && model.at(1) == '/')
				{
					model.erase(model.begin(), model.begin() + 2);
					model.insert(0, "mcpppp:item/" + folderpath);
				}
				else
				{
					model.insert(0, "mcpppp:item/");
				}
			}
			else if (option == "damage")
			{
				std::stringstream ss;
				ss.str(value);
				while (ss)
				{
					ss >> temp;
					if (temp.find('-') == std::string::npos)
					{
						damages.push_back("[" + temp + ", " + temp + "]");
					}
					else if (temp.at(0) == '-')
					{
						temp.erase(temp.begin());
						damages.push_back("<=" + temp);
					}
					else if (temp.at(temp.size() - 1) == '-')
					{
						temp.erase(temp.end() - 1);
						damages.push_back(">=" + temp);
					}
					else
					{
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								first = temp;
								first.erase(first.begin() + static_cast<std::string::difference_type>(i), first.end());
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								damages.push_back("[" + first + ", " + temp + "]");
								break;
							}
						}
					}
				}
			}
			else if (option == "damageMask")
			{
				// TODO: find out what this means
			}
			else if (option == "stackSize")
			{
				std::stringstream ss;
				ss.str(value);
				while (ss)
				{
					ss >> temp;
					if (temp.find('-') == std::string::npos)
					{
						stacksizes.push_back("[" + temp + ", " + temp + "]");
					}
					else if (temp.at(0) == '-')
					{
						temp.erase(temp.begin());
						stacksizes.push_back("<=" + temp);
					}
					else if (temp.at(temp.size() - 1) == '-')
					{
						temp.erase(temp.end() - 1);
						stacksizes.push_back(">=" + temp);
					}
					else
					{
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								first = temp;
								first.erase(first.begin() + static_cast<std::string::difference_type>(i), first.end());
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								stacksizes.push_back("[" + first + ", " + temp + "]");
								break;
							}
						}
					}
				}
			}
			else if (option == "enchantments")
			{
				std::stringstream ss;
				ss.str(value);
				while (ss)
				{
					ss >> temp;
					if (temp.find(':') == std::string::npos)
					{
						temp = "minecraft:" + temp;
					}
					enchantments.push_back(temp);
				}
			}
			else if (option == "enchantmentLevels")
			{
				std::stringstream ss;
				ss.str(value);
				while (ss)
				{
					ss >> temp;
					if (temp.find('-') == std::string::npos)
					{
						enchantmentlevels.push_back("[" + temp + ", " + temp + "]");
					}
					else if (temp.at(0) == '-')
					{
						temp.erase(temp.begin());
						enchantmentlevels.push_back("<=" + temp);
					}
					else if (temp.at(temp.size() - 1) == '-')
					{
						temp.erase(temp.end() - 1);
						enchantmentlevels.push_back(">=" + temp);
					}
					else
					{
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								first = temp;
								first.erase(first.begin() + static_cast<std::string::difference_type>(i), first.end());
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								enchantmentlevels.push_back("[" + first + ", " + temp + "]");
								break;
							}
						}
					}
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
			else if (option.find("nbt.") == 0)
			{
				temp.clear();
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
				if (temp.find("regex:") <= 1)
				{
					const bool insensitive = (temp.find("iregex:") == 0);
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = std::string("/") + (insensitive ? "(?i)" : "") + temp + "/";
				}
				else if (temp.find("pattern:") <= 1)
				{
					const bool insensitive = (temp.find("ipattern:") == 0);
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = std::string("/") + (insensitive ? "(?i)" : "") + mcpppp::oftoregex(temp) + "/";
				}
				if (mcpppp::lowercase(option) == "nbt.display.name")
				{
					name = temp;
				}
				else
				{
					tempj = { {nbt.top(), temp} };
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
		if (type != "item") // TODO: add armor later
		{
			return;
		}

		predicates.clear();
		tempj = { {"model", model} };
		if (hand != "anything")
		{
			tempj["predicate"]["entity"]["hand"] = hand;
		}
		if (name.empty())
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
			std::filesystem::create_directories(std::filesystem::u8path(path + "/assets/minecraft/overrides/item/"));
			tempj = j;
			std::ifstream fin2(std::filesystem::u8path(path + "/assets/minecraft/overrides/item/" + c + ".json"));
			if (fin2.good())
			{
				fin2 >> tempj;
				std::vector<nlohmann::json> tempv = tempj["overrides"];
				std::copy(predicates.begin(), predicates.end(), std::back_inserter(tempv));
				tempj["overrides"] = tempv;
			}
			fin2.close();
			std::ofstream fout(std::filesystem::u8path(path + "/assets/minecraft/overrides/item/" + c + ".json"));
			fout << tempj.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip)
	{
		if (mcpppp::findfolder(path.u8string(), "assets/minecraft/overrides/", zip))
		{
			if (mcpppp::autoreconvert)
			{
				out(3) << "CIM: Reconverting " << path.filename().u8string() << std::endl;
				std::filesystem::remove_all(std::filesystem::u8path(path.u8string() + "/assets/mcpppp"));
				std::filesystem::remove_all(std::filesystem::u8path(path.u8string() + "/assets/minecraft/overrides"));
			}
			else
			{
				out(2) << "CIM: Chime folder found in " << path.filename().u8string() << ", skipping" << std::endl;
				return { false, false, false };
			}
		}
		if (mcpppp::findfolder(path.u8string(), "assets/minecraft/optifine/cit/", zip))
		{
			return { true, true, false };
		}
		else if (mcpppp::findfolder(path.u8string(), "assets/minecraft/mcpatcher/cit/", zip))
		{
			return { true, false, false };
		}
		else
		{
			out(2) << "CIM: Nothing to convert in " << path.filename().u8string() << ", skipping" << std::endl;
			return { false, false, false };
		}
	}

	// main cim function
	void convert(const std::string& path, const std::string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		out(3) << "CIM: Converting Pack " << filename << std::endl;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::u8path(path + "/assets/minecraft/" + (info.optifine ? "optifine" : "mcpatcher") + "/cit")))
		{
			if (entry.path().extension() == ".png" || entry.path().extension() == ".properties" || entry.path().extension() == ".json")
			{
				out(1) << "CIM: Converting " + entry.path().filename().u8string() << std::endl;
			}
			if (entry.path().extension() == ".json" || entry.path().extension() == ".png")
			{
				other(path, entry);
			}
			else if (entry.path().extension() == ".properties")
			{
				prop(path, entry);
			}
		}
	}
};
