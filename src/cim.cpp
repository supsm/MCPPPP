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
	static std::string getfilenamehash(const std::filesystem::path& path, const bool zip)
	{
		const std::u8string u8s = path.filename().u8string() + (zip ? u8".zip" : u8"");
		return mcpppp::hash<32>(u8s.data(), u8s.size());
	}

	// converts non-properties (models and textures) to cim
	static void other(const std::filesystem::path& path, const bool zip, const std::filesystem::directory_entry& entry)
	{
		// png location (textures): assets/mcpppp_hash/textures/item
		// json location (models): assets/mcpppp_hash/models/item
		// mcpppp_hash:item/

		const std::string mcnamespace = "mcpppp_" + getfilenamehash(path, zip);
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		if (entry.path().extension() == ".png")
		{
			std::filesystem::create_directories(path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath);
			std::u8string filename = entry.path().filename().u8string();
			mcpppp::findreplace(filename, u8" ", u8"_");
			copy(entry.path(), path / u8"assets" / mbtoc8(mcnamespace) / "textures/item" / folderpath / filename);
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
						if (temp.at(0) == '.' && temp.at(1) == '/')
						{
							temp.erase(temp.begin(), temp.begin() + 2);
							temp.insert(0, mcnamespace + ":item/" + c8tomb(folderpath));
							it.value() = temp;
						}
						// std::string::contains in C++23
						if (temp.find(' ') != std::string::npos)
						{
							std::string origtemp = temp;
							mcpppp::findreplace(temp, " ", "_");
							// std::string::contains in C++23
							if (temp.find(':') == std::string::npos)
							{
								mcpppp::copy(path / u8"assets/minecraft/textures" / (mbtoc8(origtemp) + u8".png"),
									path / u8"assets" / mbtoc8(mcnamespace) / "textures/extra/minecraft" / (mbtoc8(temp) + u8".png"));
								it.value() = mcnamespace + ":extra/minecraft/" + temp;
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

	// converts cit properties to cim
	static void prop(const std::filesystem::path& path, const std::filesystem::directory_entry& entry)
	{
		std::u8string folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(u8"/cit/") + 5));
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
				if (texture.find('/') != std::string::npos && texture.at(0) != '.')
				{
					// assets/mcpppp/textures/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), path / u8"assets/mcpppp/textures/extra" / (mbtoc8(texture) + u8".png"));
					texture.insert(0, "mcpppp:extra/");
				}
				else if (texture.at(0) == '.' && texture.at(1) == '/')
				{
					texture.erase(texture.begin(), texture.begin() + 2);
					texture.insert(0, "mcpppp:item/" + c8tomb(folderpath));
				}
				else
				{
					texture.insert(0, "mcpppp:item/");
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
				if (model.find('/') != std::string::npos && model.at(0) != '.')
				{
					// assets/mcpppp/models/extra
					// mcpppp:extra/
					// if paths are specified, copy to extra folder
					mcpppp::copy(entry.path(), path / u8"assets/mcpppp/models/extra" / (mbtoc8(model) + u8".png"));
					model.insert(0, "mcpppp:extra/");
				}
				else if (model.at(0) == '.' && model.at(1) == '/')
				{
					model.erase(model.begin(), model.begin() + 2);
					model.insert(0, "mcpppp:item/" + c8tomb(folderpath));
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
					// std::string::contains in C++23
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
					// std::string::contains in C++23
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
					// std::string::contains in C++23
					if (temp.find(':') == std::string::npos)
					{
						temp.insert(0, "minecraft:");
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
					// std::string::contains in C++23
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
				if (temp.starts_with("regex:") || temp.starts_with("iregex:"))
				{
					// if first character is i, then it is iregex (case insensitive)
					const bool insensitive = (temp.front() == 'i');
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
				else if (temp.starts_with("pattern:") || temp.starts_with("iregex:"))
				{
					const bool insensitive = (temp.front() == 'i');
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					temp = std::string("/").append(insensitive ? "(?i)" : "").append(mcpppp::oftoregex(temp)).append("/");
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
		if (mcpppp::findfolder(path.generic_u8string(), u8"assets/minecraft/overrides/", zip))
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
		if (mcpppp::findfolder(path.generic_u8string(), u8"assets/minecraft/optifine/cit/", zip))
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
		else if (mcpppp::findfolder(path.generic_u8string(), u8"assets/minecraft/mcpatcher/cit/", zip))
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

	// main cim function
	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		out(3) << "CIM: Converting Pack " << c8tomb(filename) << std::endl;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path / u8"assets/minecraft" / (info.optifine ? u8"optifine" : u8"mcpatcher") / u8"cit"))
		{
			if (entry.path().extension() == ".png" || entry.path().extension() == ".properties" || entry.path().extension() == ".json")
			{
				out(1) << "CIM: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			}
			if (entry.path().extension() == ".json" || entry.path().extension() == ".png")
			{
				other(path, info.iszip, entry);
			}
			else if (entry.path().extension() == ".properties")
			{
				prop(path, entry);
			}
		}
	}
}
