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
	static void cimother(const std::string& folder, const std::string& path, const bool& zip, const std::filesystem::directory_entry& png)
	{
		// png location (textures): assets/mcpppp/textures/item
		// json location (models): assets/mcpppp/models/item
		// mcpppp:item/

		std::string folderpath = png.path().u8string();
		for (char& c : folderpath)
		{
			if (c == '\\')
			{
				c = '/';
			}
		}
		folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind("/cit/") + 5);
		folderpath.erase(folderpath.end() - png.path().filename().u8string().size(), folderpath.end());
		if (png.path().extension() == ".png")
		{
			std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/item/" + folderpath));
			std::string filename = png.path().filename().u8string();
			findreplace(filename, " ", "_");
			copy(png.path(), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/item/" + folderpath + filename));
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
							temp = "mcpppp:item/" + folderpath + temp;
							it.value() = temp;
						}
						if (temp.find(" ") != std::string::npos)
						{
							std::string origtemp = temp;
							findreplace(temp, " ", "_");
							if (temp.find(":") == std::string::npos)
							{
								supsm::copy(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/textures/" + origtemp + ".png"), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/extra/minecraft/" + temp + ".png"));
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
								supsm::copy(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/" + ns + "/textures/" + origtemp + ".png"), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/extra/" + ns + "/" + temp + ".png"));
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
			std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/item/" + folderpath));
			std::string filename = png.path().filename().u8string();
			findreplace(filename, " ", "_");
			std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/item/" + folderpath + filename));
			fout << j.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

	// converts cit properties to cim
	static void cimprop(const std::string& folder, const std::string& path, const bool& zip, const std::filesystem::directory_entry& png)
	{
		std::string folderpath = png.path().u8string();
		for (char& c : folderpath)
		{
			if (c == '\\')
			{
				c = '/';
			}
		}
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind("/cit/") + 5));
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
					supsm::copy(png.path(), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/extra/" + texture + ".png"));
					texture = "mcpppp:extra/" + texture;
				}
				else if (texture.at(0) == '.' && texture.at(1) == '/')
				{
					texture.erase(texture.begin(), texture.begin() + 2);
					texture = "mcpppp:item/" + folderpath + texture;
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
					supsm::copy(png.path(), std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/extra/" + model + ".png"));
					model = "mcpppp:extra/" + model;
				}
				else if (model.at(0) == '.' && model.at(1) == '/')
				{
					model.erase(model.begin(), model.begin() + 2);
					model = "mcpppp:item/" + folderpath + model;
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
							temp.erase(temp.begin(), temp.begin() + i + 1);
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
			std::filesystem::create_directories(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/overrides/item/"));
			tempj = j;
			std::ifstream fin2(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/overrides/item/" + c + ".json"));
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
			std::ofstream fout(std::filesystem::u8path((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/overrides/item/" + c + ".json"));
			fout << tempj.dump(1, '\t') << std::endl;
			fout.close();
		}
	}

public:
	// main cim function
	inline cim(const std::string& path, const std::string& filename, const bool& zip)
	{
		// source: assets/minecraft/*/cit (recursive)
		// destination: assets/minecraft/overrides/item

		std::string folder;
		bool optifine;
		Zippy::ZipArchive zipa;
		if (zip)
		{
			zipa.Open(path);
			if (zipa.HasEntry("assets/minecraft/overrides/"))
			{
				if (autoreconvert)
				{
					out(3) << "CIM: Reconverting " << filename << std::endl;
					zipa.DeleteEntry("assets/mcpppp/");
					zipa.DeleteEntry("assets/minecraft/overrides/");
				}
				else
				{
					out(2) << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
					return;
				}
			}
			if (zipa.HasEntry("assets/minecraft/optifine/cit/"))
			{
				optifine = true;
			}
			else if (zipa.HasEntry("assets/minecraft/mcpatcher/cit/"))
			{
				optifine = false;
			}
			else
			{
				out(2) << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
				return;
			}
			folder = filename;
			folder.erase(folder.end() - 4, folder.end());
			std::filesystem::create_directories("mcpppp-temp/" + folder);
			out(3) << "CIM: Extracting " << filename << std::endl;
			zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/cit/", "mcpppp-temp/" + folder + '/');
		}
		else
		{
			if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/overrides")))
			{
				if (autoreconvert)
				{
					out(3) << "CIM: Reconverting " << filename << std::endl;
					std::filesystem::remove_all(std::filesystem::u8path(path + "/assets/mcpppp"));
					std::filesystem::remove_all(std::filesystem::u8path(path + "/assets/minecraft/overrides"));
				}
				else
				{
					out(2) << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
					return;
				}
			}
			if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/optifine/cit")))
			{
				optifine = true;
			}
			else if (std::filesystem::is_directory(std::filesystem::u8path(path + "/assets/minecraft/mcpatcher/cit")))
			{
				optifine = false;
			}
			else
			{
				out(2) << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
				return;
			}
			out(3) << "CIM: Converting Pack " << filename << std::endl;
		}
		for (auto& png : std::filesystem::recursive_directory_iterator(std::filesystem::u8path(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit")))
		{
			if (png.path().extension() == ".png" || png.path().extension() == ".properties" || png.path().extension() == ".json")
			{
				out(1) << "CIM: Converting " + png.path().filename().u8string() << std::endl;
			}
			if (png.path().extension() == ".json" || png.path().extension() == ".png")
			{
				cimother(folder, path, zip, png);
			}
			else if (png.path().extension() == ".properties")
			{
				cimprop(folder, path, zip, png);
			}
		}
		if (zip)
		{
			out(3) << "CIM: Compressing " + filename << std::endl;
			std::string temp;
			Zippy::ZipEntryData zed;
			long long filesize;
			if (std::filesystem::exists("mcpppp-temp/" + folder + "/assets/minecraft/overrides/"))
			{
				for (auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder + "/assets/minecraft/overrides/"))
				{
					if (png.is_directory())
					{
						continue;
					}
					temp = png.path().u8string();
					temp.erase(temp.begin(), temp.begin() + folder.size() + 13);
					temp.erase(temp.end() - png.path().filename().u8string().size() - 1, temp.end()); // zippy doesnt like mixing \\ and /
					temp += '/';
					for (char& c : temp)
					{
						if (c == '\\')
						{
							c = '/';
						}
					}
					std::ifstream fin(png.path(), std::ios::binary | std::ios::ate);
					zed.clear();
					filesize = png.file_size();
					zed.resize(filesize);
					fin.seekg(0, std::ios::beg);
					fin.read(reinterpret_cast<char*>(zed.data()), filesize);
					fin.close();
					zipa.AddEntry(temp + png.path().filename().u8string() + (png.is_directory() ? "/" : ""), zed);
				}
			}
			if (std::filesystem::exists("mcpppp-temp/" + folder + "/assets/mcpppp/"))
			{
				for (auto& png : std::filesystem::recursive_directory_iterator("mcpppp-temp/" + folder + "/assets/mcpppp/"))
				{
					if (png.is_directory())
					{
						continue;
					}
					temp = png.path().u8string();
					temp.erase(temp.begin(), temp.begin() + folder.size() + 13);
					temp.erase(temp.end() - png.path().filename().u8string().size() - 1, temp.end()); // zippy doesnt like mixing \\ and /
					temp += '/';
					for (char& c : temp)
					{
						if (c == '\\')
						{
							c = '/';
						}
					}
					std::ifstream fin(png.path(), std::ios::binary | std::ios::ate);
					zed.clear();
					filesize = png.file_size();
					zed.resize(filesize);
					fin.seekg(0, std::ios::beg);
					fin.read(reinterpret_cast<char*>(zed.data()), filesize);
					fin.close();
					zipa.AddEntry(temp + png.path().filename().u8string() + (png.is_directory() ? "/" : ""), zed);
				}
			}
			temp.clear();
			zed.clear();
			zed.shrink_to_fit();
			zipa.Save();
		}
		zipa.Close();
		std::filesystem::remove_all("mcpppp-temp");
	}
};
