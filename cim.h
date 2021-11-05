/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <filesystem>
#include <stack>
#include <string>
#include <vector>

#include "utility.h"


class cim
{
private:
	// converts non-properties (models and textures) to cim
	static void cimother(const std::u8string& path, const std::filesystem::directory_entry& png)
	{
		// png location (textures): assets/mcpppp/textures/item
		// json location (models): assets/mcpppp/models/item
		// mcpppp:item/

		std::u8string folderpath = png.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind(u8"/cit/") + 5);
		folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
		if (png.path().extension() == ".png")
		{
			std::filesystem::create_directories(path + u8"/assets/mcpppp/textures/item/" + folderpath);
			std::u8string filename = png.path().filename().u8string();
			findreplace(filename, u8" ", u8"_");
			copy(png.path(), path + u8"/assets/mcpppp/textures/item/" + folderpath + filename);
		}
		else
		{
			std::ifstream fin(png.path());
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
					temp = "mcpppp:item/" + c8tomb(folderpath) + temp;
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
							temp = "mcpppp:item/" + c8tomb(folderpath) + temp;
							it.value() = temp;
						}
						if (temp.find(" ") != std::string::npos)
						{
							std::string origtemp = temp;
							findreplace(temp, " ", "_");
							if (temp.find(":") == std::string::npos)
							{
								supsm::copy(path + u8"/assets/minecraft/textures/" + mbtoc8(origtemp) + u8".png", path + u8"/assets/mcpppp/textures/extra/minecraft/" + mbtoc8(temp) + u8".png");
								it.value() = "mcpppp:extra/minecraft/" + temp;
							}
							else
							{
								std::string ns;
								for (size_t i = 0; i < origtemp.size(); i++)
								{
									if (origtemp.at(i) == ':')
									{
										origtemp.erase(origtemp.begin(), origtemp.begin() + i);
										break;
									}
									ns.push_back(origtemp.at(i));
								}
								supsm::copy(path + u8"/assets/" + mbtoc8(ns) + u8"/textures/" + mbtoc8(origtemp) + u8".png", path + u8"/assets/mcpppp/textures/extra/" + mbtoc8(ns) + u8"/" + mbtoc8(temp) + u8".png");
								it.value() = "mcpppp:extra/" + ns + "/" + temp;
							}
						}
						if (first == "")
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
			std::filesystem::create_directories(path + u8"/assets/mcpppp/models/item/" + folderpath);
			std::u8string filename = png.path().filename().u8string();
			findreplace(filename, u8" ", u8"_");
			std::ofstream fout(path + u8"/assets/mcpppp/models/item/" + folderpath + filename);
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

	// converts cit properties to cim
	static void cimprop(const std::u8string& path, const std::filesystem::directory_entry& png)
	{
		std::u8string folderpath = png.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(png.path().filename().u8string().size()), folderpath.end());
		std::string temp, option, value, type = "item", texture, model, hand = "anything", first, name;
		std::vector<std::string> items, enchantments, damages, stacksizes, enchantmentlevels;
		std::vector<nlohmann::json> nbts, predicates, tempp;
		std::stack<std::string> nbt;
		nlohmann::json tempj;
		std::ifstream fin(png.path());
		while (fin)
		{
			getline(fin, temp);
			option.clear();
			value.clear();
			bool isvalue = false;
			if (temp == "" || temp.at(0) == '#')
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
				else if (isvalue)
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
				findreplace(texture, " ", "_");
				if (texture.find(".png") != std::string::npos)
				{
					texture.erase(texture.end() - 4, texture.end());
				}
				if (texture.find("/") != std::string::npos && texture.at(0) != '.')
				{
					// assets/mcpppp/textures/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					supsm::copy(png.path(), path + u8"/assets/mcpppp/textures/extra/" + mbtoc8(texture) + u8".png");
					texture = "mcpppp:extra/" + texture;
				}
				else if (texture.at(0) == '.' && texture.at(1) == '/')
				{
					texture.erase(texture.begin(), texture.begin() + 2);
					texture = "mcpppp:item/" + c8tomb(folderpath) + texture;
				}
				else
				{
					texture = "mcpppp:item/" + texture;
				}
			}
			else if (option.starts_with("texture."))
			{
				// TODO: texture.name (idk what this means)
			}
			else if (option == "model")
			{
				model = value;
				findreplace(model, " ", "_");
				if (model.find(".json") != std::string::npos)
				{
					model.erase(model.end() - 5, model.end());
				}
				if (model.find("/") != std::string::npos && model.at(0) != '.')
				{
					// assets/mcpppp/models/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					supsm::copy(png.path(), path + u8"/assets/mcpppp/models/extra/" + mbtoc8(model) + u8".png");
					model = "mcpppp:extra/" + model;
				}
				else if (model.at(0) == '.' && model.at(1) == '/')
				{
					model.erase(model.begin(), model.begin() + 2);
					model = "mcpppp:item/" + c8tomb(folderpath) + model;
				}
				else
				{
					model = "mcpppp:item/" + model;
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
								first.erase(first.begin() + i, first.end());
								temp.erase(temp.begin(), temp.begin() + i);
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
								first.erase(first.begin() + i, first.end());
								temp.erase(temp.begin(), temp.begin() + i);
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
								first.erase(first.begin() + i, first.end());
								temp.erase(temp.begin(), temp.begin() + i);
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
			else if (option.starts_with("nbt."))
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
					const bool insensitive = (temp.starts_with("iregex:"));
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + i + 1);
							break;
						}
					}
					temp = std::string("/") + (insensitive ? "(?i)" : "") + temp + "/";
				}
				else if (temp.find("pattern:") <= 1)
				{
					const bool insensitive = (temp.starts_with("ipattern:"));
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + i + 1);
							break;
						}
					}
					temp = std::string("/") + (insensitive ? "(?i)" : "") + oftoregex(temp) + "/";
				}
				if (lowercase(option) == "nbt.display.name")
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
		if (name != "")
		{
			tempj["predicate"]["name"] = name;
		}
		predicates.push_back(tempj);
		if (nbts.size())
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
		if (enchantments.size())
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
		if (enchantmentlevels.size())
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
		if (damages.size())
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
		if (stacksizes.size())
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
			std::filesystem::create_directories(path + u8"/assets/minecraft/overrides/item/");
			tempj = j;
			std::ifstream fin2(path + u8"/assets/minecraft/overrides/item/" + mbtoc8(c) + u8".json");
			if (fin2.good())
			{
				fin2 >> tempj;
				std::vector<nlohmann::json> tempv = tempj["overrides"];
				for (const nlohmann::json& j2 : predicates)
				{
					tempv.push_back(j2);
				}
				tempj["overrides"] = tempv;
			}
			fin2.close();
			std::ofstream fout(path + u8"/assets/minecraft/overrides/item/" + mbtoc8(c) + u8".json");
			fout << tempj.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

public:
	bool success = false;

	// main cim function
	inline cim(const std::u8string& path, const std::u8string& filename)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		std::string folder;
		bool optifine;
		if (std::filesystem::is_directory(path + u8"/assets/minecraft/overrides"))
		{
			if (autoreconvert)
			{
				out(3) << "CIM: Reconverting " << filename << std::endl;
				std::filesystem::remove_all(path + u8"/assets/mcpppp");
				std::filesystem::remove_all(path + u8"/assets/minecraft/overrides");
			}
			else
			{
				out(2) << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
				return;
			}
		}
		if (std::filesystem::is_directory(path + u8"/assets/minecraft/optifine/cit"))
		{
			optifine = true;
		}
		else if (std::filesystem::is_directory(path + u8"/assets/minecraft/mcpatcher/cit"))
		{
			optifine = false;
		}
		else
		{
			out(2) << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			return;
		}
		out(3) << "CIM: Converting Pack " << filename << std::endl;
		for (auto& png : std::filesystem::recursive_directory_iterator(path + u8"/assets/minecraft/" + (optifine ? u8"optifine" : u8"mcpatcher") + u8"/cit"))
		{
			if (png.path().extension() == ".png" || png.path().extension() == ".properties" || png.path().extension() == ".json")
			{
				out(1) << u8"CIM: Converting " + png.path().filename().u8string() << std::endl;
			}
			if (png.path().extension() == ".json" || png.path().extension() == ".png")
			{
				cimother(path, png);
			}
			else if (png.path().extension() == ".properties")
			{
				cimprop(path, png);
			}
		}
		success = true;
	}
};
