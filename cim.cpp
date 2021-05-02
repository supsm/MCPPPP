/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

void cimother(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	// png location (textures): assets/mcpppp/textures/item
	// json location (models): assets/mcpppp/models/item
	// mcpppp:item/

	std::string folderpath = png.path().string();
	folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind("/cit/") + 5);
	folderpath.erase(folderpath.end() - png.path().filename().string().size(), folderpath.end());
	for (int i = 0; i < folderpath.size(); i++)
	{
		if (folderpath[i] == '\\')
		{
			folderpath[i] = '/';
		}
	}
	if (png.path().extension() == ".png")
	{
		std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/item/" + folderpath);
		std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/item/" + folderpath + png.path().filename().string());
	}
	else
	{
		std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/item/" + folderpath);
		std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/item/" + folderpath + png.path().filename().string());
	}
}

void cimprop(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	std::string folderpath = png.path().string();
	folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind("/cit/") + 5);
	folderpath.erase(folderpath.end() - png.path().filename().string().size(), folderpath.end());
	for (int i = 0; i < folderpath.size(); i++)
	{
		if (folderpath[i] == '\\')
		{
			folderpath[i] = '/';
		}
	}
	std::string temp, option, value, type = "item", texture, model, hand = "either", first;
	std::vector<std::string> items, enchantments, damages, stacksizes, enchantmentlevels;
	std::vector<nlohmann::json> nbts;
	std::ifstream fin(png.path().string());
	while (fin)
	{
		getline(fin, temp);
		option.clear();
		value.clear();
		bool isvalue = false;
		if (temp == "" || temp[0] == '#')
		{
			continue;
		}
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
		if (option == "type")
		{
			type = value;
		}
		else if (option == "items")
		{
			items.push_back(value);
		}
		else if (option == "texture")
		{
			texture = value;
			if (texture.find(".png"))
			{
				texture.erase(texture.end() - 4, texture.end());
			}
			if (texture.find("/") && texture[0] != '.')
			{
				// assets/mcpppp/textures/extra
				// mcpppp:extra/
				// if paths are specified, copy to extra folder
				std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/extra/");
				std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/textures/extra/" + texture + ".png");
				texture = "mcpppp:extra/" + texture;
			}
			else
			{
				texture = "mcpppp:item/" + texture;
			}
		}
		else if (option.find("texture") == 0)
		{
			// TODO: texture.name (idk what this means)
		}
		else if (option == "model")
		{
			model = value;
			if (model.find(".png"))
			{
				model.erase(model.end() - 4, model.end());
			}
			if (model.find("/") && model[0] != '.')
			{
				// assets/mcpppp/models/extra
				// mcpppp:extra/
				// if paths are specified, copy to extra folder
				std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/extra/");
				std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/mcpppp/models/extra/" + model + ".png");
				model = "mcpppp:extra/" + model;
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
				if (!temp.find('-'))
				{
					damages.push_back("[" + temp + ", " + temp + "]");
				}
				else if (temp[0] == '-')
				{
					temp.erase(temp.begin());
					damages.push_back("<=" + temp);
				}
				else if (temp[temp.size() - 1] == '-')
				{
					temp.erase(temp.end() - 1);
					damages.push_back(">=" + temp);
				}
				else
				{
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
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
				if (!temp.find('-'))
				{
					stacksizes.push_back("[" + temp + ", " + temp + "]");
				}
				else if (temp[0] == '-')
				{
					temp.erase(temp.begin());
					stacksizes.push_back("<=" + temp);
				}
				else if (temp[temp.size() - 1] == '-')
				{
					temp.erase(temp.end() - 1);
					stacksizes.push_back(">=" + temp);
				}
				else
				{
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
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
				if (!temp.find(':'))
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
				if (!temp.find('-'))
				{
					enchantmentlevels.push_back("[" + temp + ", " + temp + "]");
				}
				else if (temp[0] == '-')
				{
					temp.erase(temp.begin());
					enchantmentlevels.push_back("<=" + temp);
				}
				else if (temp[temp.size() - 1] == '-')
				{
					temp.erase(temp.end() - 1);
					enchantmentlevels.push_back(">=" + temp);
				}
				else
				{
					for (int i = 0; i < temp.size(); i++)
					{
						if (temp[i] == '-')
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
		else if (option.find("nbt") == 0)
		{
			
		}
	}
}

void cim(std::string path, std::string filename, bool zip)
{
	// source: assets/minecraft/*/cit (recursive)
	// destination: assets/minecraft/overrides

	std::string folder;
	bool optifine;
	Zippy::ZipArchive zipa;
	if (zip)
	{
		zipa.Open(path);
		if (zipa.HasEntry("assets/minecraft/overrides"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		else if (zipa.HasEntry("assets/minecraft/optifine/cit/"))
		{
			optifine = true;
		}
		else if (zipa.HasEntry("assets/minecraft/mcpatcher/cit/"))
		{
			optifine = false;
		}
		else
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "CIM: Extracting " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "CIM: Extracting " << filename << std::endl;
		}
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/cit/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/minecraft/overrides"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/optifine/cit"))
		{
			optifine = true;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/mcpatcher/cit"))
		{
			optifine = false;
		}
		else
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "CIM: Converting Pack " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "CIM: Converting Pack " << filename << std::endl;
		}
	}
	for (auto& png : std::filesystem::recursive_directory_iterator(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit"))
	{
		if (png.path().extension() == ".png" || png.path().extension() == ".properties" || png.path().extension() == ".json")
		{
			if (outputlevel <= 1)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Converting " + png.path().filename().string() << std::endl;
			}
			if (logfile.good() && loglevel <= 1)
			{
				logfile << timestamp() << "CIM: Converting " + png.path().filename().string() << std::endl;
			}
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
}